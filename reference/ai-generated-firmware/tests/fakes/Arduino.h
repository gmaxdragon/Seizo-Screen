// Offline test substitutes only; never used by PlatformIO target builds.
#pragma once
#include <stdint.h>
#include <stddef.h>
constexpr int INPUT=0,OUTPUT=1,LOW=0,HIGH=1;
extern int fake_levels[64],fake_modes[64],fake_gpio_reads,fake_delay_us;
inline void pinMode(int p,int mode){fake_modes[p]=mode;}
inline void digitalWrite(int p,int level){fake_levels[p]=level;}
inline int digitalRead(int p){++fake_gpio_reads;return fake_levels[p];}
inline void delayMicroseconds(unsigned n){fake_delay_us+=(int)n;}
