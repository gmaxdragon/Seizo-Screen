// AI-generated reference integration; no display/touch hardware was tested.
// Register sequences adapted from Waveshare LCD_Driver.c V1.2 (2019-08-16).
// See THIRD-PARTY-NOTICES.txt and vendor source archive for license/provenance.
#include "display_ui.h"
#include "vendor_init.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#if defined(SEIZO_DISPLAY_ST7789) == defined(SEIZO_DISPLAY_HX8347D)
#error Select exactly one vendor-derived display-controller profile in platformio.ini
#endif
namespace seizo {
namespace {
constexpr int LCD_CS=11,DC=12,RST=13,BL=14,TP_CS=15,IRQ=18;
constexpr uint16_t BG=0x0841,WHITE=0xffff,CYAN=0x56dc,RED=0xf800,GREEN=0x07e0,GRAY=0x4208,YELLOW=0xffe0;
const uint8_t letters[26][5]={
 {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},
 {0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},{0x7f,0x09,0x09,0x09,0x01},
 {0x3e,0x41,0x49,0x49,0x7a},{0x7f,0x08,0x08,0x08,0x7f},{0x00,0x41,0x7f,0x41,0x00},
 {0x20,0x40,0x41,0x3f,0x01},{0x7f,0x08,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
 {0x7f,0x02,0x0c,0x02,0x7f},{0x7f,0x04,0x08,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},
 {0x7f,0x09,0x09,0x09,0x06},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,0x09,0x19,0x29,0x46},
 {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7f,0x01,0x01},{0x3f,0x40,0x40,0x40,0x3f},
 {0x1f,0x20,0x40,0x20,0x1f},{0x7f,0x20,0x18,0x20,0x7f},{0x63,0x14,0x08,0x14,0x63},
 {0x03,0x04,0x78,0x04,0x03},{0x61,0x51,0x49,0x45,0x43}};
const uint8_t digits[10][5]={
 {0x3e,0x51,0x49,0x45,0x3e},{0x00,0x42,0x7f,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},
 {0x21,0x41,0x45,0x4b,0x31},{0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
 {0x3c,0x4a,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},{0x36,0x49,0x49,0x49,0x36},
 {0x06,0x49,0x49,0x29,0x1e}};
uint32_t checksum(const TouchCalibration& c) {
  const uint8_t* p=(const uint8_t*)&c; uint32_t h=2166136261u;
  for(size_t i=0;i<offsetof(TouchCalibration,checksum);++i) h=(h^p[i])*16777619u;
  return h;
}
}
DisplayUi::DisplayUi(SPIClass& spi,const int* cs,size_t count,SpiLock lock,SpiUnlock unlock,void* context)
 :spi_(spi),all_cs_(cs),count_(count),lock_(lock),unlock_(unlock),context_(context) {}
void DisplayUi::deselect(){for(size_t i=0;i<count_;++i)digitalWrite(all_cs_[i],HIGH);}
bool DisplayUi::start(uint32_t hz) {
  if(!lock_||!unlock_||!lock_(context_))return false;
  deselect();spi_.beginTransaction(SPISettings(hz,MSBFIRST,SPI_MODE0));return true;
}
void DisplayUi::finish(){deselect();spi_.endTransaction();unlock_(context_);}
bool DisplayUi::command(uint8_t c,const uint8_t* p,size_t n) {
  if(!start(4000000))return false;
  digitalWrite(LCD_CS,LOW);digitalWrite(DC,LOW);spi_.transfer(c);digitalWrite(DC,HIGH);
  for(size_t i=0;i<n;++i)spi_.transfer(p[i]);finish();return true;
}
void DisplayUi::begin(){
  pinMode(DC,INPUT);pinMode(RST,INPUT);pinMode(BL,INPUT);pinMode(IRQ,INPUT);
  nvs_ok_=preferences_.begin("seizo-touch",false);
  calibrating_=!loadCalibration();wait_release_=true;
}
bool DisplayUi::initialize(uint32_t now){
  // Reset and vendor settling delays are cooperative: the main loop keeps
  // sampling inputs, enforcing link timeouts, and checking LCD VIO throughout.
  if(init_phase_==0){
    pinMode(DC,OUTPUT);pinMode(RST,OUTPUT);pinMode(BL,OUTPUT);digitalWrite(BL,LOW);
    digitalWrite(RST,HIGH);init_at_=now;init_phase_=1;return false;
  }
  if(init_phase_==1){if((uint32_t)(now-init_at_)<5)return false;digitalWrite(RST,LOW);init_at_=now;init_phase_=2;return false;}
  if(init_phase_==2){if((uint32_t)(now-init_at_)<10)return false;digitalWrite(RST,HIGH);init_at_=now;init_phase_=3;return false;}
  if(init_phase_==3){if((uint32_t)(now-init_at_)<150)return false;init_phase_=4;init_index_=0;}
#ifdef SEIZO_DISPLAY_ST7789
  const InitOp* ops=st7789_init;const size_t n=sizeof(st7789_init)/sizeof(InitOp);
#else
  const InitOp* ops=hx8347d_init;const size_t n=sizeof(hx8347d_init)/sizeof(InitOp);
#endif
  if(init_phase_==5){
    if((uint32_t)(now-init_at_)<ops[init_index_-1].delay_ms)return false;
    init_phase_=4;
  }
  if(init_index_<n){
    const InitOp& op=ops[init_index_];
    if(!command(op.cmd,op.data,op.length))return false;
    ++init_index_;init_at_=now;init_phase_=5;return false;
  }
  digitalWrite(BL,HIGH);next_row_=0;flushing_=false;dirty_=true;return true;
}
void DisplayUi::rectangle(int x,int y,int w,int h,uint16_t color){
  for(int yy=y;yy<y+h&&yy<Height;++yy)if(yy>=0)
    for(int xx=x;xx<x+w&&xx<Width;++xx)if(xx>=0)frame_[yy*Width+xx]=color;
}
void DisplayUi::glyph(int x,int y,char ch,uint16_t color,int scale){
  if(ch>='a'&&ch<='z')ch-=32;
  uint8_t p[5]={};
  if(ch>='A'&&ch<='Z')memcpy(p,letters[ch-'A'],5);
  else if(ch>='0'&&ch<='9')memcpy(p,digits[ch-'0'],5);
  else if(ch==':'){p[2]=0x24;}
  else if(ch=='.'){p[2]=0x40;}
  else if(ch=='-'||ch=='_'){for(int i=0;i<5;++i)p[i]=ch=='-'?0x08:0x40;}
  else if(ch=='/'){p[0]=0x60;p[1]=0x10;p[2]=0x08;p[3]=0x04;p[4]=0x03;}
  else if(ch=='?'){p[0]=2;p[1]=1;p[2]=0x51;p[3]=9;p[4]=6;}
  for(int a=0;a<5;++a)for(int b=0;b<7;++b)if(p[a]&(1u<<b))rectangle(x+a*scale,y+b*scale,scale,scale,color);
}
void DisplayUi::text(int x,int y,const char* s,uint16_t color,int scale){
  for(;*s&&x<Width;s++,x+=6*scale)glyph(x,y,*s,color,scale);
}
void DisplayUi::paint(const Snapshot& s){
  rectangle(0,0,320,240,BG);
  // Simple vector mark uses the user's two-triangle Seizo motif.
  for(int row=0;row<12;++row){rectangle(7+12-row,3+row,2*row+1,1,CYAN);rectangle(7+row,17+row,25-2*row,1,CYAN);}
  text(43,8,"SEIZO",CYAN,2);
#ifdef SEIZO_DISPLAY_ST7789
  text(170,7,"ST7789 / REF",GRAY);
#else
  text(170,7,"HX8347D / REF",GRAY);
#endif
  text(170,19,"NO ARM COMMANDS",YELLOW);
  if(calibrating_){
    text(10,42,"TOUCH CALIBRATION",WHITE,2);
    text(10,67,"TAP CROSS AND LIFT BETWEEN POINTS",YELLOW);
    char msg[48];snprintf(msg,sizeof msg,"POINT %d OF 5",calibration_index_+1);text(10,83,msg,CYAN);
    const int px[]={20,300,20,300,160},py[]={20,20,220,220,120};
    const int x=px[calibration_index_],y=py[calibration_index_];
    rectangle(x-9,y,19,1,RED);rectangle(x,y-9,1,19,RED);
    return;
  }
  char line[64];
  snprintf(line,sizeof line,"HOST:%s  IO:%s  BB:%s",s.host_ok?"LINK":"OFF",s.io_ok?"OK":"FAULT",s.breadboard_ok?"OK":"FAULT");text(8,37,line,s.host_ok&&s.io_ok&&s.breadboard_ok?GREEN:RED);
  text(8,53,s.needs_ack?"PANEL INHIBITED - ACK REQUIRED":"PANEL INPUTS ENABLED",s.needs_ack?YELLOW:GREEN);
  rectangle(8,70,96,27,s.hold?RED:GRAY);text(15,80,s.hold?"HOLD REQ":"HOLD IDLE",WHITE);
  rectangle(112,70,96,27,s.stop1?RED:GRAY);text(119,80,s.stop1?"STOP1 REQ":"STOP1 IDLE",WHITE);
  rectangle(216,70,96,27,s.stop2?RED:GRAY);text(223,80,s.stop2?"STOP2 REQ":"STOP2 IDLE",WHITE);
  text(8,103,"REQUESTS ONLY / NOT A SAFETY STOP",YELLOW);
  if(s.meter==MeterState::Valid){snprintf(line,sizeof line,"DC: %ld.%03ld V",(long)(s.millivolts/1000),(long)(s.millivolts%1000));text(8,119,line,CYAN,2);}
  else{snprintf(line,sizeof line,"DC: -- %s",meterStateName(s.meter));text(8,122,line,YELLOW);}
  text(8,140,"NOMINAL REF / ACCURACY UNVERIFIED",GRAY);
  for(int k=0;k<10;++k){
    int bit=k<7?k:k+4;bool pressed=(s.pressed&(1u<<bit))!=0;
    int x=8+(k%5)*63,y=154+(k/5)*25;
    rectangle(x,y,54,20,pressed?CYAN:GRAY);snprintf(line,sizeof line,"B%d %s",k+1,pressed?"ON":"--");text(x+3,y+7,line,pressed?BG:WHITE);
  }
  rectangle(6,210,98,27,GRAY);text(13,220,"ACK PANEL",WHITE);
  rectangle(110,210,98,27,GRAY);text(117,220,config_prompt_?"CONFIRM CFG":"METER CFG",config_prompt_?YELLOW:WHITE);
  rectangle(214,210,100,27,GRAY);text(221,220,"CAL TOUCH",WHITE);
  if(config_prompt_){rectangle(6,113,308,37,BG);text(10,117,"CONFIRM DEFAULT GAIN 1.000 / ZERO 0",YELLOW);text(10,131,"THIS DOES NOT CALIBRATE ACCURACY",WHITE);}
}
void DisplayUi::flushChunk(){
  if(!flushing_||!start(4000000))return;
  const uint16_t first=next_row_,last=(uint16_t)(first+1);
  digitalWrite(LCD_CS,LOW);
  auto cmd=[this](uint8_t c){digitalWrite(DC,LOW);spi_.transfer(c);digitalWrite(DC,HIGH);};
  auto data16=[this](uint16_t v){spi_.transfer((uint8_t)(v>>8));spi_.transfer((uint8_t)v);};
#ifdef SEIZO_DISPLAY_ST7789
  cmd(0x2a);data16(0);data16(239);cmd(0x2b);data16(first);data16(last);cmd(0x2c);
#else
  auto reg=[this,&cmd](uint8_t c,uint8_t v){cmd(c);spi_.transfer(v);};
  reg(0x02,0);reg(0x03,0);reg(0x04,0);reg(0x05,239);
  reg(0x06,(uint8_t)(first>>8));reg(0x07,(uint8_t)first);
  reg(0x08,(uint8_t)(last>>8));reg(0x09,(uint8_t)last);cmd(0x22);
#endif
  // Vendor native portrait240x320 addressing; software rotation avoids guessed MADCTL.
  for(uint16_t py=first;py<=last;++py)for(uint16_t px=0;px<240;++px){
    uint16_t color=frame_[(239-px)*320+py];data16(color);
  }
  finish();next_row_+=2;if(next_row_>=320){next_row_=0;flushing_=false;}
}
uint16_t DisplayUi::touchSample(uint8_t c){
  digitalWrite(TP_CS,LOW);spi_.transfer(c);uint16_t h=spi_.transfer(0);uint16_t l=spi_.transfer(0);digitalWrite(TP_CS,HIGH);
  return (uint16_t)(((h<<8)|l)>>3)&0x0fff;
}
bool DisplayUi::readTouch(uint16_t& x,uint16_t& y){
  if(digitalRead(IRQ)!=LOW||!start(1000000))return false;
  uint16_t a[5],b[5];
  for(int i=0;i<5;++i){a[i]=touchSample(0x90);b[i]=touchSample(0xd0);}
  finish();
  for(int i=0;i<4;++i)for(int j=i+1;j<5;++j){if(a[j]<a[i]){uint16_t t=a[j];a[j]=a[i];a[i]=t;}if(b[j]<b[i]){uint16_t t=b[j];b[j]=b[i];b[i]=t;}}
  x=(a[1]+a[2]+a[3])/3;y=(b[1]+b[2]+b[3])/3;
  return digitalRead(IRQ)==LOW&&x>50&&x<4045&&y>50&&y<4045;
}
bool DisplayUi::loadCalibration(){
  if(!nvs_ok_||preferences_.getBytesLength("affine")!=sizeof calibration_)return false;
  if(preferences_.getBytes("affine",&calibration_,sizeof calibration_)!=sizeof calibration_)return false;
  if(calibration_.magic!=0x535a5431||calibration_.checksum!=checksum(calibration_))return false;
  const float* p=&calibration_.a;for(int i=0;i<6;++i)if(!isfinite(p[i]))return false;
  return fabs(calibration_.a)<2&&fabs(calibration_.b)<2&&fabs(calibration_.d)<2&&fabs(calibration_.e)<2&&fabs(calibration_.c)<1000&&fabs(calibration_.f)<1000;
}
void DisplayUi::saveCalibration(){calibration_.checksum=checksum(calibration_);if(nvs_ok_)preferences_.putBytes("affine",&calibration_,sizeof calibration_);}
void DisplayUi::processTouch(PanelCore& core,uint32_t now){
  uint16_t x,y;bool down=readTouch(x,y);
  if(!down){pen_down_=false;wait_release_=false;ack_start_=0;return;}
  if(wait_release_)return;
  if(!pen_down_){pen_down_=true;previous_x_=x;previous_y_=y;stable_since_=now;ack_start_=0;}
  if(abs((int)x-previous_x_)>40||abs((int)y-previous_y_)>40){previous_x_=x;previous_y_=y;stable_since_=now;ack_start_=0;return;}
  if((uint32_t)(now-stable_since_)<40)return;
  if(calibrating_){
    samples_[calibration_index_++]={(float)x,(float)y};wait_release_=true;dirty_=true;
    if(calibration_index_==5){
      TouchCalibration candidate;
      if(fitTouch(samples_,candidate)){calibration_=candidate;saveCalibration();calibrating_=false;}
      calibration_index_=0;
    }
    return;
  }
  float sx,sy;if(!calibration_.map(x,y,sx,sy)||sy<210){ack_start_=0;return;}
  if(sx<105){
    if(!ack_start_)ack_start_=now;
    if((uint32_t)(now-ack_start_)>=800){core.acknowledge(now);dirty_=true;wait_release_=true;}
  }else if(sx<210){
    ack_start_=0;
    if(config_prompt_){core.handleHostLine("CFG 1000000 0",now);config_prompt_=false;}
    else config_prompt_=true;
    dirty_=true;wait_release_=true;
  }else{
    ack_start_=0;
    calibrating_=true;calibration_index_=0;wait_release_=true;dirty_=true;config_prompt_=false;
  }
}
void DisplayUi::tick(PanelCore& core,uint32_t now){
  Snapshot s=core.snapshot();
  if(!s.io_ok||!s.lcd_ok){
    if(vio_was_){digitalWrite(BL,LOW);pinMode(BL,INPUT);pinMode(DC,INPUT);pinMode(RST,INPUT);}
    ready_=false;vio_was_=false;flushing_=false;init_phase_=0;wait_release_=true;ack_start_=0;return;
  }
  if(!vio_was_){vio_was_=true;init_phase_=0;wait_release_=true;}
  if(!ready_){ready_=initialize(now);if(!ready_)return;}
  if((uint32_t)(now-last_touch_)>=20){last_touch_=now;processTouch(core,now);}
  if(!flushing_&&(dirty_||(uint32_t)(now-last_paint_)>=250)){
    paint(core.snapshot());dirty_=false;last_paint_=now;flushing_=true;next_row_=0;
  }
  flushChunk();
}
}
