#pragma once
#include <stdint.h>
#include <vector>
constexpr int MSBFIRST=1,SPI_MODE0=0;
struct SPISettings{uint32_t hz;int order,mode;SPISettings(uint32_t h,int o,int m):hz(h),order(o),mode(m){}};
class SPIClass {
 public:
  SPISettings last{0,0,0};bool active=false;std::vector<uint8_t> sent;size_t cursor=0;
  void beginTransaction(SPISettings s){last=s;active=true;sent.clear();cursor=0;}
  uint8_t transfer(uint8_t n){sent.push_back(n);const uint8_t response[]={0,0x0a,0xbc};return response[(cursor++)%3];}
  void endTransaction(){active=false;}
};
