// Reference touch calibration math. No hardware validation.
#pragma once
#include <stdint.h>
#include <math.h>
namespace seizo {
struct TouchPoint { float x,y; };
struct TouchCalibration {
  uint32_t magic=0; float a=0,b=0,c=0,d=0,e=0,f=0; uint32_t checksum=0;
  bool map(float rx,float ry,float& x,float& y)const {
    x=a*rx+b*ry+c; y=d*rx+e*ry+f;
    return isfinite(x)&&isfinite(y)&&x>=0&&x<320&&y>=0&&y<240;
  }
};
inline bool fitTouch(const TouchPoint* raw,TouchCalibration& out) {
  // Targets: TL(20,20), TR(300,20), BL(20,220), BR(300,220), C(160,120).
  const double ux=raw[1].x-raw[0].x,uy=raw[1].y-raw[0].y;
  const double vx=raw[2].x-raw[0].x,vy=raw[2].y-raw[0].y;
  const double det=ux*vy-vx*uy;
  if(!isfinite(det)||fabs(det)<100000.0) return false;
  out.a=(float)(280.0*vy/det); out.b=(float)(-280.0*vx/det);
  out.d=(float)(-200.0*uy/det); out.e=(float)(200.0*ux/det);
  out.c=20-out.a*raw[0].x-out.b*raw[0].y;
  out.f=20-out.d*raw[0].x-out.e*raw[0].y;
  float x,y;
  if(!out.map(raw[3].x,raw[3].y,x,y)||fabs(x-300)>20||fabs(y-220)>20) return false;
  if(!out.map(raw[4].x,raw[4].y,x,y)||fabs(x-160)>20||fabs(y-120)>20) return false;
  out.magic=0x535a5431; return true;
}
}
