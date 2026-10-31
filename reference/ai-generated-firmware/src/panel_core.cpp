// AI-generated reference firmware. No physical or actuator validation.
#include "panel_core.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

namespace seizo {
namespace {
constexpr uint16_t KEY_MASK=0x387f; // A0..6 and B3..5.
constexpr uint16_t STOP_MASK=0x0700;
bool number(const char*& p, long& out) {
  if (*p!=' ') return false;
  while (*p==' ') ++p;
  if (!*p || (*p!='-' && (*p<'0'||*p>'9'))) return false;
  errno=0; char* end=nullptr; out=strtol(p,&end,10);
  if (errno==ERANGE || end==p) return false;
  p=end; return true;
}
bool done(const char* p) { while (*p==' ') ++p; return *p==0; }
}
const char* meterStateName(MeterState s) {
  switch(s) {
    case MeterState::Disabled:return "disabled"; case MeterState::Unconfigured:return "unconfigured"; case MeterState::Settling:return "settling";
    case MeterState::Valid:return "valid"; case MeterState::IoFault:return "io_fault";
    case MeterState::ReferenceFault:return "reference_fault"; case MeterState::OutOfRange:return "out_of_range";
  } return "invalid";
}
PanelCore::PanelCore(PanelIo& io,LineWriter writer,void* context):io_(io),writer_(writer),context_(context) {}
void PanelCore::output(const char* line) { if (writer_) writer_(line,context_); }
void PanelCore::error(const char* code) { char b[80]; snprintf(b,sizeof b,"{\"type\":\"error\",\"code\":\"%s\"}",code); output(b); }
void PanelCore::inhibit() {
  state_.needs_ack=true;
  blocked_=KEY_MASK; // A held key cannot become a fresh action after a fault.
}
bool PanelCore::begin(uint32_t now) {
  state_=Snapshot(); state_.ms=now; state_.io_ok=configured_=io_.begin();
  begun_=true; seeded_=false; blocked_=KEY_MASK; meter_enabled_=false;
  last_poll_=now-5; last_state_=now; last_meter_=now;
  output("{\"type\":\"hello\",\"proto\":1,\"reference_firmware\":true}");
  tick(now); return state_.io_ok;
}
void PanelCore::emitEvent(const char* kind,int index,bool down) {
  char b[160];
  snprintf(b,sizeof b,"{\"type\":\"event\",\"nonce\":%lu,\"seq\":%lu,\"ms\":%lu,\"kind\":\"%s\",\"index\":%d,\"down\":%s}",
    (unsigned long)state_.session,(unsigned long)++state_.sequence,(unsigned long)state_.ms,kind,index,down?"true":"false");
  output(b);
}
void PanelCore::updateInputs(uint16_t raw,uint32_t now) {
  state_.raw=raw;
  if (!seeded_) {
    candidate_=state_.inputs=raw;
    for(int i=0;i<16;++i) changed_[i]=now;
    blocked_=KEY_MASK; seeded_=true;
  }
  const uint16_t previous=state_.inputs;
  for(int i=0;i<16;++i) {
    const uint16_t bit=(uint16_t)(1u<<i);
    if ((raw&bit)!=(candidate_&bit)) { candidate_^=bit; changed_[i]=now; }
    if ((uint32_t)(now-changed_[i])>=DebounceMs) {
      state_.inputs=(uint16_t)((state_.inputs&~bit)|(candidate_&bit));
    }
  }
  // Fault assertion uses raw hardware immediately; deassertion remains debounced.
  state_.lcd_ok=(raw&0x0080) && (state_.inputs&0x0080);
  state_.breadboard_ok=(raw&0x8000) && (state_.inputs&0x8000);
  state_.hold=!(raw&0x0100) || !(state_.inputs&0x0100);
  // STOP1/2 are normally-closed dry contacts to GND. Opening a contact or
  // breaking its wire raises the pulled-up input and asserts the request.
  state_.stop1=(raw&0x0200) || (state_.inputs&0x0200);
  state_.stop2=(raw&0x0400) || (state_.inputs&0x0400);
  if (!state_.lcd_ok || !state_.breadboard_ok || state_.hold || state_.stop1 || state_.stop2) inhibit();
  state_.pressed=(uint16_t)(~state_.inputs & KEY_MASK);
  blocked_ &= (uint16_t)(~state_.inputs); // Release arms that key for a future press.
  const uint16_t difference=(uint16_t)(previous^state_.inputs);
  for(int i=0;i<16;++i) {
    const uint16_t bit=(uint16_t)(1u<<i);
    if (!(difference&bit)) continue;
    bool down=!(state_.inputs&bit);
    if (KEY_MASK&bit) {
      int index=i<7?i+1:i-3; // B3(bit11)=>8.
      if (state_.host_ok && !state_.needs_ack && !(blocked_&bit)) emitEvent("key",index,down);
    } else if (STOP_MASK&bit) {
      if(i==9||i==10)down=!down; // NC contact open/high means STOP asserted.
      if (state_.host_ok) emitEvent(i==8?"hold":i==9?"stop1":"stop2",0,down);
    }
  }
}
void PanelCore::updateMeter(uint32_t now) {
  if(!state_.io_ok){meter_enabled_=false;state_.meter=MeterState::IoFault;state_.millivolts=0;return;}
  bool enabled=state_.io_ok && !(state_.raw&0x4000) && !(state_.inputs&0x4000);
  if (!enabled) { meter_enabled_=false; state_.meter=MeterState::Disabled; state_.millivolts=0; return; }
  if (!state_.meter_configured) { meter_enabled_=false; state_.meter=MeterState::Unconfigured; state_.millivolts=0; return; }
  if (!meter_enabled_) { meter_enabled_=true; meter_since_=now; state_.meter=MeterState::Settling; }
  if ((uint32_t)(now-meter_since_)<MeterSettleMs) return;
  if ((uint32_t)(now-last_meter_)<50) return;
  last_meter_=now;
  uint16_t discard=0, ref=0, raw=0;
  // Discard the first conversion after each channel change, then average8 samples.
  uint32_t total0=0,total1=0;
  if(!io_.readAdc(1,discard)) { state_.meter=MeterState::IoFault; return; }
  for(int i=0;i<8;++i) { if(!io_.readAdc(1,ref)){state_.meter=MeterState::IoFault;return;} total1+=ref; }
  if(!io_.readAdc(0,discard)) { state_.meter=MeterState::IoFault; return; }
  for(int i=0;i<8;++i) { if(!io_.readAdc(0,raw)){state_.meter=MeterState::IoFault;return;} total0+=raw; }
  state_.adc0=(uint16_t)((total0+4)/8); state_.reference=(uint16_t)((total1+4)/8);
  if(state_.reference<2800 || state_.reference>3400 || state_.adc0>=4090) {
    state_.meter=MeterState::ReferenceFault; state_.millivolts=0; return;
  }
  // Ratiometric use of CH1's nominal2.5V reference; gain/zero are user settings,
  // not evidence of calibration or accuracy. Divider gain=(49.9+49.9+10)/10=10.98.
  double mv=(double)state_.adc0*2500.0*10.98/state_.reference;
  mv=mv*state_.gain_ppm/1000000.0+state_.zero_mv;
  if(!isfinite(mv) || mv<0 || mv>24000) { state_.meter=MeterState::OutOfRange; state_.millivolts=0; return; }
  state_.millivolts=(int32_t)lround(mv); state_.meter=MeterState::Valid;
}
void PanelCore::tick(uint32_t now) {
  if(!begun_) return;
  state_.ms=now;
  if(state_.host_ok && (uint32_t)(now-last_ping_)>HostTimeoutMs) {
    state_.host_ok=false; state_.session=0; state_.sequence=0; inhibit();
    output("{\"type\":\"link\",\"up\":false}");
  }
  if((uint32_t)(now-last_poll_)>=5) {
    last_poll_=now; uint16_t raw=0xffff;
    if(!configured_) configured_=io_.begin();
    if(configured_ && io_.readInputs(raw)) { state_.io_ok=true; updateInputs(raw,now); }
    else { configured_=false; state_.io_ok=false; state_.lcd_ok=false; state_.breadboard_ok=false; seeded_=false; inhibit(); }
  }
  updateMeter(now);
  if((uint32_t)(now-last_state_)>=250) { last_state_=now; sendStatus(); }
}
bool PanelCore::acknowledge(uint32_t now) {
  tick(now); state_.ms=now;
  if(!state_.host_ok || !state_.io_ok || !state_.lcd_ok || !state_.breadboard_ok ||
      state_.hold || state_.stop1 || state_.stop2 || state_.pressed) return false;
  state_.needs_ack=false; blocked_=0; return true;
}
void PanelCore::handleHostLine(const char* line,uint32_t now) {
  tick(now); state_.ms=now;
  if(!line || strlen(line)>MaxHostLine) { error("line_length"); return; }
  for(const char* q=line;*q;++q) if((unsigned char)*q<32 || (unsigned char)*q>126){error("ascii_only");return;}
  const char* p=line; long a=0,b=0;
  if(!strncmp(p,"PING",4)) {
    p+=4;
    // Bound to31bits so strtol is identical on32bitArduino and64bitnative.
    if(!number(p,a)||!done(p)||a<=0||a>2147483647L){error("ping");return;}
    bool fresh=!state_.host_ok || state_.session!=(uint32_t)a;
    if(fresh) { state_.sequence=0; inhibit(); }
    state_.session=(uint32_t)a; state_.host_ok=true; last_ping_=now;
    char out[100]; snprintf(out,sizeof out,"{\"type\":\"pong\",\"proto\":1,\"nonce\":%lu}",(unsigned long)state_.session); output(out);
  } else if(!strcmp(p,"STATUS")) sendStatus();
  else if(!strncmp(p,"CFG",3)) {
    p+=3;
    if(!state_.host_ok || !number(p,a)||!number(p,b)||!done(p)||a<500000||a>1500000||b<-1000||b>1000){error("cfg");return;}
    state_.gain_ppm=(int32_t)a; state_.zero_mv=(int32_t)b; state_.meter_configured=true; state_.meter=MeterState::Settling; meter_enabled_=false; sendStatus();
  } else if(!strncmp(p,"ACK",3)) {
    p+=3;
    if(!number(p,a)||!done(p)||(uint32_t)a!=state_.session||!acknowledge(now)){error("ack_inhibited");return;}
    sendStatus();
  } else error("unknown_command");
}
void PanelCore::sendStatus() {
  char out[512], mv[24];
  if(state_.meter==MeterState::Valid) snprintf(mv,sizeof mv,"%ld",(long)state_.millivolts); else strcpy(mv,"null");
  snprintf(out,sizeof out,"{\"type\":\"state\",\"proto\":1,\"nonce\":%lu,\"seq\":%lu,\"ms\":%lu,\"inputs\":%u,\"pressed\":%u,\"io_ok\":%s,\"host_ok\":%s,\"lcd_ok\":%s,\"bb_ok\":%s,\"hold\":%s,\"stop1\":%s,\"stop2\":%s,\"needs_ack\":%s,\"meter\":\"%s\",\"mv\":%s,\"adc0\":%u,\"ref\":%u,\"gain_ppm\":%ld,\"zero_mv\":%ld,\"meter_configured\":%s,\"accuracy_verified\":false}",
    (unsigned long)state_.session,(unsigned long)state_.sequence,(unsigned long)state_.ms,
    state_.inputs,state_.pressed,state_.io_ok?"true":"false",state_.host_ok?"true":"false",state_.lcd_ok?"true":"false",state_.breadboard_ok?"true":"false",
    state_.hold?"true":"false",state_.stop1?"true":"false",state_.stop2?"true":"false",state_.needs_ack?"true":"false",meterStateName(state_.meter),mv,
    state_.adc0,state_.reference,(long)state_.gain_ppm,(long)state_.zero_mv,state_.meter_configured?"true":"false");
  output(out);
}
}

#ifdef ARDUINO
namespace seizo {
ArduinoPanelIo::ArduinoPanelIo(TwoWire& wire,SPIClass& spi,int adc_cs,const int* all_cs,size_t count,int lcd_vio_pin,int bb_fault_pin,SpiLock lock,SpiUnlock unlock,void* context)
 :wire_(wire),spi_(spi),adc_cs_(adc_cs),all_cs_(all_cs),count_(count),lcd_vio_pin_(lcd_vio_pin),bb_fault_pin_(bb_fault_pin),lock_(lock),unlock_(unlock),context_(context) {}
bool ArduinoPanelIo::writeReg(uint8_t reg,uint8_t value) {
  wire_.beginTransmission(0x20); wire_.write(reg); wire_.write(value); return wire_.endTransmission()==0;
}
bool ArduinoPanelIo::readReg(uint8_t reg,uint8_t& value) {
  wire_.beginTransmission(0x20); wire_.write(reg);
  if(wire_.endTransmission(false)!=0 || wire_.requestFrom((uint8_t)0x20,(uint8_t)1)!=(uint8_t)1) return false;
  value=(uint8_t)wire_.read(); return true;
}
bool ArduinoPanelIo::begin() {
  if((lock_==nullptr)!=(unlock_==nullptr) || count_>16 || (!all_cs_ && count_)) return false;
  if(lcd_vio_pin_<0 || bb_fault_pin_<0 || lcd_vio_pin_==bb_fault_pin_)return false;
  // External100k pulldowns on the revised HAT define both disconnected inputs.
  // Do not enable uncertain internal pulls against the panel10k FAULT pullup.
  pinMode(lcd_vio_pin_,INPUT);pinMode(bb_fault_pin_,INPUT);
  digitalWrite(adc_cs_,HIGH); pinMode(adc_cs_,OUTPUT);
  for(size_t i=0;i<count_;++i) { digitalWrite(all_cs_[i],HIGH); pinMode(all_cs_[i],OUTPUT); }
  // BANK0, sequential addressing. This module exclusively owns MCP configuration.
  // Current MCP23017 DS20001952D: A7/B7 are OUTPUT ONLY. Set their latches
  // low before changing directions; both physical pins are unused/NC.
  bool ok=writeReg(0x0a,0x00) && writeReg(0x14,0x00) && writeReg(0x15,0x00) &&
    writeReg(0x00,0x7f) && writeReg(0x01,0x7f) && writeReg(0x02,0x00) &&
    writeReg(0x03,0x00) && writeReg(0x0c,0x7f) && writeReg(0x0d,0x7f);
  const uint8_t regs[]={0x0a,0x00,0x01,0x02,0x03,0x0c,0x0d,0x14,0x15};
  const uint8_t expected[]={0x00,0x7f,0x7f,0x00,0x00,0x7f,0x7f,0x00,0x00};
  for(size_t i=0;ok && i<sizeof(regs);++i) { uint8_t value=0; ok=readReg(regs[i],value) && value==expected[i]; }
  return ok;
}
bool ArduinoPanelIo::readInputs(uint16_t& value) {
  // Detect even a short physical expander reset between polling intervals.
  // Never proceed to GPIO reads with A7/B7 accidentally restored to inputs.
  uint8_t a_dir=0,b_dir=0;
  if(!readReg(0x00,a_dir)||!readReg(0x01,b_dir)||a_dir!=0x7f||b_dir!=0x7f)return false;
  wire_.beginTransmission(0x20); wire_.write(0x12);
  if(wire_.endTransmission(false)!=0) return false;
  if(wire_.requestFrom((uint8_t)0x20,(uint8_t)2)!=(uint8_t)2) { while(wire_.available()) wire_.read(); return false; }
  uint8_t a=(uint8_t)wire_.read(), b=(uint8_t)wire_.read();
  value=composePanelInputs((uint16_t)a|((uint16_t)b<<8),digitalRead(lcd_vio_pin_)==HIGH,digitalRead(bb_fault_pin_)==HIGH);
  return true;
}
bool ArduinoPanelIo::readAdc(uint8_t channel,uint16_t& value) {
  if(channel>1 || (lock_ && !lock_(context_))) return false;
  for(size_t i=0;i<count_;++i) digitalWrite(all_cs_[i],HIGH);
  digitalWrite(adc_cs_,HIGH);
  spi_.beginTransaction(SPISettings(100000,MSBFIRST,SPI_MODE0));
  digitalWrite(adc_cs_,LOW);
  delayMicroseconds(1); // MCP3202 tSUCS>=100ns; explicit margin before SCLK.
  spi_.transfer(0x01);
  uint8_t high=spi_.transfer((uint8_t)(0xa0|(channel<<6)));
  uint8_t low=spi_.transfer(0);
  digitalWrite(adc_cs_,HIGH);
  delayMicroseconds(1); // MCP3202 tCSH>=500ns between conversions.
  spi_.endTransaction();
  if(unlock_) unlock_(context_);
  value=(uint16_t)((high&0x0f)<<8)|low; return true;
}
}
#endif
