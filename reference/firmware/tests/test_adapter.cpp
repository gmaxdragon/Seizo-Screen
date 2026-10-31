// Reference offline register/GPIO/protocol mock. No physical interfaces.
#include "../src/panel_core.h"
#include <assert.h>
#include <stdio.h>
int fake_levels[64]={},fake_modes[64]={},fake_gpio_reads=0,fake_delay_us=0;
int locks=0,unlocks=0;
bool lock(void*){++locks;return true;}void unlock(void*){++unlocks;}
int main(){
  TwoWire wire;SPIClass spi;const int cs[]={11,15,21};
  seizo::ArduinoPanelIo io(wire,spi,21,cs,3,6,7,lock,unlock,nullptr);
  assert(io.begin());
  assert(wire.regs[0x00]==0x7f&&wire.regs[0x01]==0x7f);
  assert(wire.regs[0x14]==0&&wire.regs[0x15]==0);
  assert(wire.regs[0x0c]==0x7f&&wire.regs[0x0d]==0x7f);
  assert(fake_modes[6]==INPUT&&fake_modes[7]==INPUT);
  wire.regs[0x12]=0xff;wire.regs[0x13]=0xff; // Physical bit7 values must be discarded.
  uint16_t value=0;assert(io.readInputs(value)&&value==0x7f7f);
  fake_levels[6]=HIGH;assert(io.readInputs(value)&&value==0x7fff);
  fake_levels[7]=HIGH;assert(io.readInputs(value)&&value==0xffff);
  fake_levels[6]=LOW;assert(io.readInputs(value)&&value==0xff7f);
  // A missed short reset restores IODIR=FF. Detect it before reading GPIO;
  // never read potentially transitioning output-only pins as inputs.
  int before=wire.gpio_requests;wire.regs[0]=0xff;
  assert(!io.readInputs(value)&&wire.gpio_requests==before);
  assert(io.begin());wire.nack=true;assert(!io.readInputs(value));wire.nack=false;
  assert(io.readAdc(0,value)&&value==0xabc);
  assert(spi.sent.size()==3&&spi.sent[0]==1&&spi.sent[1]==0xa0&&spi.sent[2]==0);
  assert(spi.last.hz==100000&&spi.last.mode==SPI_MODE0&&!spi.active);
  assert(io.readAdc(1,value)&&spi.sent[1]==0xe0);
  assert(locks==unlocks&&fake_delay_us==4);
  for(int p:cs)assert(fake_levels[p]==HIGH);
  assert(!io.readAdc(2,value));
  puts("Arduino adapter erratum/register/direct-GPIO/ADC mocks passed");
}
