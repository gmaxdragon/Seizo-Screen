#!/usr/bin/env python3
"""Reference: reads panel inputs and prints JSON. No motor adapters.

Requires pyserial when run against an explicitly selected port. Importing this
module and running its unit tests never discovers or opens any serial port.
"""
from __future__ import annotations
import argparse
import json
import secrets
import time
from typing import Callable

MAX_DEVICE_LINE = 512
MAX_HOST_LINE = 79
BOOL_FIELDS = ("io_ok", "host_ok", "lcd_ok", "bb_ok", "hold", "stop1", "stop2", "needs_ack", "meter_configured")
METER_STATES = {"disabled", "unconfigured", "settling", "valid", "io_fault", "reference_fault", "out_of_range"}

class ProtocolError(ValueError):
    pass

def integer(value, low, high):
    return type(value) is int and low <= value <= high

def no_duplicates(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ProtocolError("duplicate JSON key")
        result[key] = value
    return result

def parse_message(raw: bytes):
    if not raw or len(raw) > MAX_DEVICE_LINE or b"\x00" in raw:
        raise ProtocolError("invalid frame length")
    try:
        value = json.loads(raw.decode("ascii"), object_pairs_hook=no_duplicates,
                           parse_constant=lambda x: (_ for _ in ()).throw(ProtocolError("nonfinite JSON")))
    except (UnicodeError, json.JSONDecodeError, RecursionError) as exc:
        raise ProtocolError("invalid JSON") from exc
    if type(value) is not dict or type(value.get("type")) is not str:
        raise ProtocolError("message object required")
    kind = value["type"]
    if kind == "hello":
        if not integer(value.get("proto"),1,1) or value.get("reference_firmware") is not True:
            raise ProtocolError("incompatible hello")
    elif kind == "pong":
        if not integer(value.get("proto"),1,1) or not integer(value.get("nonce"), 1, 0x7fffffff):
            raise ProtocolError("invalid pong")
    elif kind == "state":
        if not integer(value.get("proto"),1,1) or not integer(value.get("nonce"), 0, 0x7fffffff):
            raise ProtocolError("invalid state protocol/session")
        for key in ("seq", "ms"):
            if not integer(value.get(key), 0, 0xffffffff):
                raise ProtocolError("invalid state counter")
        for key in ("inputs", "pressed"):
            if not integer(value.get(key), 0, 0xffff):
                raise ProtocolError("invalid input mask")
        if any(type(value.get(key)) is not bool for key in BOOL_FIELDS):
            raise ProtocolError("invalid boolean")
        if value.get("meter") not in METER_STATES or value.get("accuracy_verified") is not False:
            raise ProtocolError("invalid meter metadata")
        if value["meter"] == "valid":
            if value.get("meter_configured") is not True:
                raise ProtocolError("meter defaults have not been confirmed")
            if not integer(value.get("mv"), 0, 24000):
                raise ProtocolError("invalid voltage")
        elif value.get("mv") is not None:
            raise ProtocolError("invalid meter must be null")
        if not all(integer(value.get(k), 0, 4095) for k in ("adc0", "ref")):
            raise ProtocolError("invalid ADC code")
        if not integer(value.get("gain_ppm"), 500000, 1500000) or not integer(value.get("zero_mv"), -1000, 1000):
            raise ProtocolError("invalid calibration configuration")
    elif kind == "event":
        if not integer(value.get("nonce"), 1, 0x7fffffff) or not integer(value.get("seq"), 1, 0xffffffff):
            raise ProtocolError("invalid event session/sequence")
        if not integer(value.get("ms"), 0, 0xffffffff) or type(value.get("down")) is not bool:
            raise ProtocolError("invalid event timestamp/level")
        if value.get("kind") == "key":
            if not integer(value.get("index"), 1, 10):
                raise ProtocolError("invalid key")
        elif value.get("kind") not in {"hold", "stop1", "stop2"} or not integer(value.get("index"),0,0):
            raise ProtocolError("invalid event kind")
    elif kind == "link":
        if value.get("up") is not False:
            raise ProtocolError("invalid link message")
    elif kind == "error":
        if type(value.get("code")) is not str or len(value["code"]) > 48:
            raise ProtocolError("invalid error")
    else:
        raise ProtocolError("unknown message type")
    return value

class LineFramer:
    """Bounded buffering; discards a complete overlong frame before recovery."""
    def __init__(self):
        self.buffer = bytearray()
        self.discarding = False

    def feed(self, chunk: bytes):
        lines = []
        for byte in chunk:
            if byte == 10:
                if not self.discarding and self.buffer:
                    lines.append(bytes(self.buffer).removesuffix(b"\r"))
                self.buffer.clear()
                self.discarding = False
            elif not self.discarding:
                if len(self.buffer) >= MAX_DEVICE_LINE:
                    self.buffer.clear()
                    self.discarding = True
                else:
                    self.buffer.append(byte)
        return lines

class Bridge:
    """Session validator. Output is read-only telemetry, never robot commands."""
    def __init__(self, nonce=None):
        self.nonce = nonce if nonce is not None else secrets.randbelow(0x7ffffffe) + 1
        if not integer(self.nonce, 1, 0x7fffffff):
            raise ValueError("invalid nonce")
        self.connected = False
        self.last_seq = 0
        self.last_rx = 0.0
        self.connected_since = 0.0
        self.last_state_rx = None
        self.state = None

    def ping(self):
        return f"PING {self.nonce}\n".encode("ascii")

    def disconnect(self):
        self.connected = False
        self.last_seq = 0
        self.state = None
        self.last_state_rx = None

    def receive(self, raw: bytes, now: float):
        try:
            message = parse_message(raw)
        except ProtocolError:
            self.disconnect()
            raise
        kind = message["type"]
        if kind in {"hello", "link"}:
            self.disconnect()
        elif kind == "pong":
            if message["nonce"] != self.nonce:
                self.disconnect()
                raise ProtocolError("stale pong")
            if not self.connected:
                self.connected_since = now
            self.connected = True
            self.last_rx = now
        elif kind in {"state", "event"}:
            if not self.connected:
                return None  # Unsolicited telemetry can precede our first PONG.
            if message["nonce"] != self.nonce:
                self.disconnect()
                raise ProtocolError("unconfirmed or stale session")
            if kind == "event":
                if message["seq"] != self.last_seq + 1:
                    self.disconnect()
                    raise ProtocolError("event gap/replay; explicit reconnect required")
                self.last_seq = message["seq"]
            else:
                if message["seq"] != self.last_seq:
                    self.disconnect()
                    raise ProtocolError("state/event sequence mismatch")
                self.state = message
                self.last_state_rx = now
            self.last_rx = now
        return message

    def expired(self, now):
        state_time = self.last_state_rx if self.last_state_rx is not None else self.connected_since
        if self.connected and (now - self.last_rx > 2.0 or now - state_time > 2.0):
            self.disconnect()
            return True
        return False

def publish(message):
    print(json.dumps(message, separators=(",", ":"), allow_nan=False), flush=True)

def run(port: str, baud: int, serial_factory=None, stop: Callable[[], bool] = lambda: False):
    # No automatic discovery, no motor-controller imports, no HTTP/stop endpoint.
    if serial_factory is None:
        import serial
        serial_factory = serial.Serial
    while not stop():
        bridge = Bridge()  # New nonce on every reconnect; never auto-ACK.
        framer = LineFramer()
        try:
            with serial_factory(port=port, baudrate=baud, timeout=0.1, write_timeout=0.5,
                                rtscts=False, dsrdtr=False) as stream:
                stream.reset_input_buffer()
                last_ping = -1.0
                opened = time.monotonic()
                while not stop():
                    now = time.monotonic()
                    if now-last_ping >= 0.5:
                        stream.write(bridge.ping())
                        last_ping=now
                    for line in framer.feed(stream.read(256)):
                        was_connected = bridge.connected
                        try:
                            message = bridge.receive(line, time.monotonic())
                        except ProtocolError as exc:
                            publish({"type":"bridge_protocol_error", "detail":str(exc)})
                            if was_connected and not bridge.connected:
                                raise ConnectionError("session reset") from exc
                            continue
                        if message is not None:
                            publish(message)
                    if bridge.expired(time.monotonic()):
                        raise ConnectionError("panel timeout")
                    if not bridge.connected and time.monotonic()-opened > 3.0:
                        raise ConnectionError("panel handshake timeout")
        except KeyboardInterrupt:
            return
        except Exception as exc:
            bridge.disconnect()
            publish({"type":"bridge_link", "up":False, "detail":str(exc)[:160]})
            if not stop():
                time.sleep(1.0)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="Explicit panel port; never auto-scanned")
    parser.add_argument("--baud", type=int, default=115200)
    args=parser.parse_args()
    if not 1200 <= args.baud <= 2000000:
        parser.error("unsupported baud")
    run(args.port,args.baud)

if __name__ == "__main__":
    main()
