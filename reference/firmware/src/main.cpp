// Reference firmware. USB/UART panel telemetry only; no arm adapters.
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "panel_core.h"
#include "display_ui.h"

namespace {
const int all_cs[]={11,15,21};
SemaphoreHandle_t spi_mutex=nullptr;
bool take_spi(void*){return spi_mutex&&xSemaphoreTake(spi_mutex,pdMS_TO_TICKS(5))==pdTRUE;}
void release_spi(void*){xSemaphoreGive(spi_mutex);}
#if defined(SEIZO_HOST_UART) && SEIZO_HOST_UART
HardwareSerial host_port(1);
#else
#define host_port Serial
#endif
// Nonblocking whole-line queue; a dropped event causes host sequence validation
// to reconnect and require acknowledgement instead of silently replaying input.
char tx[4096];size_t tx_read=0,tx_write=0;
void write_line(const char* line,void*){
  size_t n=strlen(line);if(n>=512)return;
  size_t free=(tx_read+sizeof(tx)-tx_write-1)%sizeof(tx);if(free<n+1)return;
  for(size_t i=0;i<n;++i){tx[tx_write]=line[i];tx_write=(tx_write+1)%sizeof(tx);}
  tx[tx_write]='\n';tx_write=(tx_write+1)%sizeof(tx);
}
void flush_serial(){
  int available=host_port.availableForWrite();
  size_t n=(tx_write+sizeof(tx)-tx_read)%sizeof(tx);
  if(available<=0||n==0)return;
  n=min(n,(size_t)available);n=min(n,(size_t)128);n=min(n,sizeof(tx)-tx_read);
  size_t sent=host_port.write((const uint8_t*)tx+tx_read,n);tx_read=(tx_read+sent)%sizeof(tx);
}
// Revised HAT J4 pin3=GPIO6 LCD_VIO_PRESENT, pin4=GPIO7 BB_FAULT_N.
seizo::ArduinoPanelIo io(Wire,SPI,21,all_cs,3,6,7,take_spi,release_spi,nullptr);
seizo::PanelCore core(io,write_line,nullptr);
seizo::DisplayUi display(SPI,all_cs,3,take_spi,release_spi,nullptr);
char rx[seizo::PanelCore::MaxHostLine+1];size_t rx_length=0;bool discard=false;
void read_serial(uint32_t now){
  int budget=96;
  while(budget--&&host_port.available()){
    int c=host_port.read();if(c<0)return;
    if(c=='\n'){
      if(!discard){if(rx_length&&rx[rx_length-1]=='\r')--rx_length;rx[rx_length]=0;core.handleHostLine(rx,now);}
      rx_length=0;discard=false;
    }else if(!discard){
      if((c<32&&c!='\r')||c>126||rx_length>=seizo::PanelCore::MaxHostLine){discard=true;rx_length=0;}
      else rx[rx_length++]=(char)c;
    }
  }
}
}
void setup(){
  for(int pin:all_cs){digitalWrite(pin,HIGH);pinMode(pin,OUTPUT);}
  spi_mutex=xSemaphoreCreateMutex();
#if defined(SEIZO_HOST_UART) && SEIZO_HOST_UART
  host_port.begin(115200,SERIAL_8N1,44,43);
#else
  host_port.begin(115200);
#endif
  Wire.begin(17,16,100000);Wire.setTimeOut(10);
  SPI.begin(10,8,9,-1);
  core.begin(millis());display.begin();
}
void loop(){
  uint32_t now=millis();core.tick(now);read_serial(now);display.tick(core,now);flush_serial();delay(1);
}
