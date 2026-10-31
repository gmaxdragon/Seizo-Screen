#pragma once
#include <stdint.h>
#include <vector>
class TwoWire {
 public:
  uint8_t regs[256]={},pointer=0; bool nack=false; std::vector<uint8_t> tx,rx;size_t cursor=0;int gpio_requests=0;
  void beginTransmission(uint8_t){tx.clear();}
  size_t write(uint8_t n){tx.push_back(n);return 1;}
  uint8_t endTransmission(bool=true){
    if(nack)return 2;
    if(tx.empty())return 3;
    pointer=tx[0];for(size_t i=1;i<tx.size();++i)regs[pointer++]=tx[i];return 0;
  }
  uint8_t requestFrom(uint8_t,uint8_t count){
    rx.clear();cursor=0;if(nack)return 0;if(pointer==0x12)++gpio_requests;
    for(uint8_t i=0;i<count;++i){rx.push_back(regs[pointer++]);}
    return count;
  }
  int read(){return cursor<rx.size()?rx[cursor++]:-1;}
  int available(){return (int)(rx.size()-cursor);}
};
