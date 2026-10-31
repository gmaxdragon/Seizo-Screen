"""Offline protocol tests; imports/opening no serial devices."""
import importlib.util
import json
from pathlib import Path
import unittest

FILE=Path(__file__).resolve().parents[1]/"host"/"seizo_panel_bridge.py"
SPEC=importlib.util.spec_from_file_location("bridge",FILE)
B=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(B)

def frame(value): return json.dumps(value,separators=(",",":")).encode()
def state(**kw):
    value=dict(type="state",proto=1,nonce=123,seq=0,ms=100,inputs=65535,pressed=0,
               io_ok=True,host_ok=True,lcd_ok=True,bb_ok=True,hold=False,stop1=False,
               stop2=False,needs_ack=True,meter="disabled",mv=None,adc0=0,ref=0,
               gain_ppm=1000000,zero_mv=0,meter_configured=True,accuracy_verified=False)
    value.update(kw); return value
def event(**kw):
    value=dict(type="event",nonce=123,seq=1,ms=500,kind="key",index=1,down=True)
    value.update(kw); return value

class ParserTests(unittest.TestCase):
    def test_state(self): self.assertEqual(B.parse_message(frame(state()))["mv"],None)
    def test_good_voltage(self): self.assertEqual(B.parse_message(frame(state(meter="valid",mv=24000)))["mv"],24000)
    def test_voltage_requires_explicit_configuration(self):
        for configured in (False, 1, None):
            with self.subTest(configured=configured), self.assertRaises(B.ProtocolError):
                B.parse_message(frame(state(meter="valid",mv=1000,meter_configured=configured)))
        self.assertIsNone(B.parse_message(frame(state(meter="unconfigured",meter_configured=False)))["mv"])
    def test_reject_bad_voltage_and_types(self):
        for kw in (dict(meter="valid",mv=-1),dict(meter="valid",mv=24001),dict(mv=0),dict(pressed=True),dict(proto=True),dict(io_ok=1),dict(accuracy_verified=True),dict(ref=4096),dict(gain_ppm=1)):
            with self.subTest(kw=kw), self.assertRaises(B.ProtocolError): B.parse_message(frame(state(**kw)))
    def test_malformed(self):
        for data in (b"",b"x"*513,b"{",b"[]",b'{}',b'{"type":"state","type":"pong"}',b'{"type":"error","code":NaN}',b'\xff',b'\x00',b'{"type":"motor"}'):
            with self.subTest(data=data[:40]), self.assertRaises(B.ProtocolError): B.parse_message(data)
    def test_event_bounds(self):
        for kw in (dict(index=0),dict(index=11),dict(seq=0),dict(down=1),dict(nonce=0),dict(kind="stop1",index=False)):
            with self.subTest(kw=kw), self.assertRaises(B.ProtocolError): B.parse_message(frame(event(**kw)))
    def test_framer_fragment_and_multiple(self):
        f=B.LineFramer(); self.assertEqual(f.feed(b"a"),[])
        self.assertEqual(f.feed(b"b\r\nc\n"),[b"ab",b"c"])
    def test_framer_oversize_recovery(self):
        f=B.LineFramer(); self.assertEqual(f.feed(b"x"*600+b"\nvalid\n"),[b"valid"])
        self.assertLessEqual(len(f.buffer),512)

class SessionTests(unittest.TestCase):
    def setUp(self): self.b=B.Bridge(123)
    def pong(self): self.b.receive(frame(dict(type="pong",proto=1,nonce=123)),1.0)
    def test_only_ping_outbound(self): self.assertEqual(self.b.ping(),b"PING 123\n")
    def test_unsolicited_state_before_pong(self): self.assertIsNone(self.b.receive(frame(state(nonce=0)),0))
    def test_reject_stale_pong(self):
        with self.assertRaises(B.ProtocolError): self.b.receive(frame(dict(type="pong",proto=1,nonce=99)),1)
        self.assertFalse(self.b.connected)
    def test_stale_pong_clears_existing_state(self):
        self.pong(); self.b.receive(frame(state()),1.1)
        with self.assertRaises(B.ProtocolError): self.b.receive(frame(dict(type="pong",proto=1,nonce=99)),1.2)
        self.assertFalse(self.b.connected); self.assertIsNone(self.b.state)
    def test_state_and_sequence(self):
        self.pong(); self.b.receive(frame(state()),1.1)
        self.b.receive(frame(event()),1.2)
        self.b.receive(frame(state(seq=1)),1.3)
        self.assertEqual(self.b.last_seq,1)
    def test_event_gap_disconnect_clears_state(self):
        self.pong(); self.b.receive(frame(state()),1.1)
        with self.assertRaises(B.ProtocolError): self.b.receive(frame(event(seq=2)),1.2)
        self.assertFalse(self.b.connected); self.assertIsNone(self.b.state); self.assertEqual(self.b.last_seq,0)
    def test_replayed_event(self):
        self.pong(); self.b.receive(frame(event()),1.1)
        with self.assertRaises(B.ProtocolError): self.b.receive(frame(event()),1.2)
    def test_timeout_clears_session(self):
        self.pong(); self.b.receive(frame(state()),1.1)
        self.assertTrue(self.b.expired(3.2)); self.assertFalse(self.b.connected); self.assertIsNone(self.b.state)
    def test_hello_and_link_reset(self):
        for msg in (dict(type="hello",proto=1,reference_firmware=True),dict(type="link",up=False)):
            self.pong(); self.b.receive(frame(msg),2); self.assertFalse(self.b.connected)
    def test_stale_session_doesnt_publish(self):
        self.pong()
        with self.assertRaises(B.ProtocolError): self.b.receive(frame(event(nonce=999)),2)
        self.assertEqual(self.b.last_seq,0)
        self.assertFalse(self.b.connected)
    def test_corrupt_frame_after_handshake_clears_state(self):
        self.pong(); self.b.receive(frame(state()),1.1)
        with self.assertRaises(B.ProtocolError): self.b.receive(b"not json",1.2)
        self.assertFalse(self.b.connected); self.assertIsNone(self.b.state)
    def test_pong_does_not_hide_missing_state(self):
        self.pong(); self.b.receive(frame(state()),1.1)
        self.b.receive(frame(dict(type="pong",proto=1,nonce=123)),3.0)
        self.assertTrue(self.b.expired(3.2)); self.assertIsNone(self.b.state)

if __name__=="__main__": unittest.main()
