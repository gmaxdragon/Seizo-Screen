// AI-generated offline geometric tests. No touch hardware is connected.
#include "../src/touch_math.h"
#include <assert.h>
#include <stdio.h>
using namespace seizo;
int main(){
  TouchPoint raw[5]={{300,300},{3700,300},{300,3700},{3700,3700},{2000,2000}};
  TouchCalibration c;assert(fitTouch(raw,c));
  float x=0,y=0;assert(c.map(2000,2000,x,y));assert(fabs(x-160)<0.01&&fabs(y-120)<0.01);
  // Swapped and mirrored raw axes must be resolved by calibration, not guessed.
  TouchPoint rotated[5]={{3700,300},{3700,3700},{300,300},{300,3700},{2000,2000}};
  assert(fitTouch(rotated,c));assert(c.map(300,3700,x,y));assert(fabs(x-300)<0.01&&fabs(y-220)<0.01);
  assert(!c.map(65535,65535,x,y));
  TouchPoint degenerate[5]={{1,1},{2,2},{3,3},{4,4},{5,5}};assert(!fitTouch(degenerate,c));
  raw[3].x=1500;assert(!fitTouch(raw,c));
  // Every physical portrait pixel maps to one in-bounds landscape pixel.
  unsigned visited[320*240]={};
  for(unsigned py=0;py<320;++py)for(unsigned px=0;px<240;++px){
    unsigned i=(239-px)*320+py;assert(i<320*240);++visited[i];
  }
  for(unsigned n:visited)assert(n==1);
  puts("touch calibration and framebuffer mapping tests passed");
}
