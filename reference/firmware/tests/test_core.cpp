// Reference offline mock tests. Compile/run with a native C++11 compiler.
#include "../src/panel_core.h"
#include <assert.h>
#include <string>
#include <vector>
#include <stdio.h>
using namespace seizo;
struct Mock:PanelIo {
  uint16_t inputs=0xf9ff, raw0=1000, ref=3103; bool good=true; int adc_reads=0;
  bool begin()override{return good;}
  bool readInputs(uint16_t& x)override{x=inputs;return good;}
  bool readAdc(uint8_t ch,uint16_t& x)override{++adc_reads;x=ch?ref:raw0;return good;}
};
void capture(const char* p,void* c){((std::vector<std::string>*)c)->push_back(p);}
size_t count(const std::vector<std::string>& v,const char* word){size_t n=0;for(auto&s:v)if(s.find(word)!=std::string::npos)++n;return n;}
void healthy(PanelCore& p){p.handleHostLine("PING 123",1);p.tick(30);assert(p.acknowledge(30));}
int main(){
  {
    assert(composePanelInputs(0xffff,false,false)==0x7f7f);
    assert(composePanelInputs(0,false,true)==0x8000);
    assert(composePanelInputs(0,true,false)==0x0080);
    assert(composePanelInputs(0x7f7f,true,true)==0xffff);
  }
  {
    Mock m;std::vector<std::string>v;PanelCore p(m,capture,&v);assert(p.begin(0));healthy(p);
    m.inputs&=~1u;p.tick(35);p.tick(50);assert(count(v,"\"kind\":\"key\"")==0);
    p.tick(55);assert(count(v,"\"kind\":\"key\"")==1);
    m.inputs|=1u;p.tick(60);p.tick(80);assert(count(v,"\"kind\":\"key\"")==2);
    p.tick(1600);assert(!p.snapshot().host_ok&&p.snapshot().needs_ack&&p.snapshot().sequence==0);
    p.handleHostLine("PING 456",1610);assert(p.snapshot().needs_ack);
  }
  {
    Mock m;m.inputs&=~1u;std::vector<std::string>v;PanelCore p(m,capture,&v);p.begin(0);
    p.handleHostLine("PING 123",1);assert(!p.acknowledge(30));p.tick(100);assert(count(v,"\"kind\":\"key\"")==0);
    m.inputs|=1u;p.tick(105);p.tick(125);assert(p.acknowledge(125));
  }
  {
    Mock m;std::vector<std::string>v;PanelCore p(m,capture,&v);p.begin(0);healthy(p);
    m.inputs&=~0x4000u;p.tick(35);p.tick(55);p.tick(200);p.sendStatus();
    assert(!p.snapshot().meter_configured&&p.snapshot().meter==MeterState::Unconfigured);
    assert(m.adc_reads==0&&v.back().find("\"mv\":null")!=std::string::npos);
  }
  {
    Mock m;std::vector<std::string>v;PanelCore p(m,capture,&v);p.begin(0);healthy(p);
    m.inputs|=0x0200u;p.tick(35);assert(p.snapshot().stop1&&p.snapshot().needs_ack);
    m.inputs&=~0x0200u;p.tick(40);p.tick(60);assert(p.snapshot().needs_ack);
    assert(p.acknowledge(60));m.good=false;p.tick(65);assert(!p.snapshot().io_ok&&p.snapshot().needs_ack);
    m.good=true;p.tick(70);assert(p.snapshot().needs_ack);
  }
  {
    Mock m;std::vector<std::string>v;PanelCore p(m,capture,&v);p.begin(0);healthy(p);
    p.handleHostLine("CFG 1000000 0",30);
    m.inputs&=~0x4000u;p.tick(35);p.tick(55);assert(p.snapshot().meter==MeterState::Settling);
    p.tick(104);assert(m.adc_reads==0);p.tick(105);assert(p.snapshot().meter==MeterState::Valid);
    assert(p.snapshot().millivolts>8800&&p.snapshot().millivolts<8900);
    m.ref=0;p.tick(155);p.sendStatus();assert(p.snapshot().meter==MeterState::ReferenceFault);
    assert(v.back().find("\"mv\":null")!=std::string::npos);
    m.inputs|=0x4000u;p.tick(160);assert(p.snapshot().meter==MeterState::Disabled);
  }
  {
    Mock m;std::vector<std::string>v;PanelCore p(m,capture,&v);p.begin(0);healthy(p);
    const char* bad[]={"CFG 0 0","CFG 1000000 1001","PING -1","PING 123 junk","ACK 999","BOGUS"};
    for(auto s:bad){p.handleHostLine(s,50);}
    assert(count(v,"\"type\":\"error\"")==6);
    p.handleHostLine("CFG 1100000 -5",60);assert(p.snapshot().gain_ppm==1100000&&p.snapshot().zero_mv==-5);
    assert(p.snapshot().meter!=MeterState::Valid);
  }
  {
    Mock m;std::vector<std::string>v;PanelCore p(m,capture,&v);p.begin(0);healthy(p);
    m.inputs&=~0x0080u;p.tick(35);assert(!p.snapshot().lcd_ok&&p.snapshot().needs_ack);
    m.inputs|=0x0080u;p.tick(40);p.tick(60);assert(p.snapshot().lcd_ok&&p.snapshot().needs_ack);
    assert(p.acknowledge(60));m.inputs&=~0x8000u;p.tick(65);
    assert(!p.snapshot().breadboard_ok&&p.snapshot().needs_ack);
  }
  {
    Mock m;std::vector<std::string>v;PanelCore p(m,capture,&v);p.begin(0xfffffff0u);
    p.handleHostLine("PING 123",0xfffffff1u);p.tick(20);assert(p.acknowledge(20));
    p.tick(1600);assert(!p.snapshot().host_ok&&p.snapshot().needs_ack);
  }
  puts("panel_core offline mock tests passed");
}
