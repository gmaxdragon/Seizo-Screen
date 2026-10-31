// AI-generated reference firmware. No physical or actuator validation.
#pragma once
#include <stdint.h>
#include <stddef.h>

namespace seizo {
// Bits7/15 are logical transport slots, not MCP23017 input pins. The actual
// MCP23017 A7/B7 must remain outputs per current DS20001952D/erratum.
inline uint16_t composePanelInputs(uint16_t mcp14,bool lcd_vio,bool bb_fault_high){
  return (uint16_t)((mcp14&0x7f7f)|(lcd_vio?0x0080:0)|(bb_fault_high?0x8000:0));
}
class PanelIo {
 public:
  virtual ~PanelIo() {}
  virtual bool begin() = 0;
  virtual bool readInputs(uint16_t& value) = 0;
  virtual bool readAdc(uint8_t channel, uint16_t& value) = 0;
};
typedef void (*LineWriter)(const char*, void*);
enum class MeterState : uint8_t { Disabled, Unconfigured, Settling, Valid, IoFault, ReferenceFault, OutOfRange };
struct Snapshot {
  uint32_t ms=0, session=0, sequence=0;
  uint16_t inputs=0xffff, raw=0xffff, pressed=0;
  bool io_ok=false, host_ok=false, lcd_ok=false, breadboard_ok=false;
  bool hold=false, stop1=false, stop2=false, needs_ack=true;
  bool meter_configured=false;
  MeterState meter=MeterState::Disabled;
  uint16_t adc0=0, reference=0;
  int32_t millivolts=0, gain_ppm=1000000, zero_mv=0;
};
class PanelCore {
 public:
  static constexpr size_t MaxHostLine=79;
  static constexpr uint32_t HostTimeoutMs=1500, DebounceMs=20, MeterSettleMs=50;
  PanelCore(PanelIo& io, LineWriter writer, void* context=nullptr);
  bool begin(uint32_t now);
  void tick(uint32_t now);
  void handleHostLine(const char* line, uint32_t now);
  void onHostLine(const char* line, uint32_t now) { handleHostLine(line,now); }
  Snapshot snapshot() const { return state_; }
  // Only a deliberate local operator action may call this; never on reconnect.
  bool acknowledge(uint32_t now);
  void sendStatus();
 private:
  PanelIo& io_; LineWriter writer_; void* context_;
  Snapshot state_;
  uint16_t candidate_=0xffff, blocked_=0xffff;
  uint32_t changed_[16]={}, last_ping_=0, last_poll_=0, last_state_=0, meter_since_=0, last_meter_=0;
  bool seeded_=false, meter_enabled_=false, begun_=false, configured_=false;
  void output(const char* line);
  void inhibit();
  void updateInputs(uint16_t raw, uint32_t now);
  void updateMeter(uint32_t now);
  void emitEvent(const char* kind, int index, bool down);
  void error(const char* code);
};
const char* meterStateName(MeterState s);
}

#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
namespace seizo {
typedef bool (*SpiLock)(void*);
typedef void (*SpiUnlock)(void*);
class ArduinoPanelIo : public PanelIo {
 public:
  // Caller begins Wire/SPI and shares the same lock with display/touch adapters.
  ArduinoPanelIo(TwoWire& wire, SPIClass& spi, int adc_cs, const int* all_cs,
                 size_t count, int lcd_vio_pin, int bb_fault_pin,
                 SpiLock lock=nullptr, SpiUnlock unlock=nullptr, void* context=nullptr);
  bool begin() override;
  bool readInputs(uint16_t& value) override;
  bool readAdc(uint8_t channel, uint16_t& value) override;
 private:
  TwoWire& wire_; SPIClass& spi_; int adc_cs_; const int* all_cs_; size_t count_;
  int lcd_vio_pin_,bb_fault_pin_;
  SpiLock lock_; SpiUnlock unlock_; void* context_;
  bool writeReg(uint8_t address, uint8_t value);
  bool readReg(uint8_t address, uint8_t& value);
};
}
#endif
