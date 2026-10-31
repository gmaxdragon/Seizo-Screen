// Reference adapter; Waveshare init sequences retain maker license.
#pragma once
#include "panel_core.h"
#include "touch_math.h"
#include <Preferences.h>

namespace seizo {
class DisplayUi {
 public:
  DisplayUi(SPIClass& spi,const int* all_cs,size_t count,SpiLock lock,SpiUnlock unlock,void* context);
  void begin();
  void tick(PanelCore& core,uint32_t now);
 private:
  static constexpr int Width=320,Height=240;
  SPIClass& spi_; const int* all_cs_; size_t count_; SpiLock lock_; SpiUnlock unlock_; void* context_;
  uint16_t frame_[Width*Height]={};
  bool ready_=false, vio_was_=false, dirty_=true, flushing_=false;
  uint8_t init_phase_=0; size_t init_index_=0; uint32_t init_at_=0;
  uint16_t next_row_=0;
  uint32_t last_paint_=0,last_touch_=0,ack_start_=0,stable_since_=0;
  bool pen_down_=false,wait_release_=true,config_prompt_=false,calibrating_=true;
  int calibration_index_=0;
  uint16_t previous_x_=0,previous_y_=0;
  TouchPoint samples_[5]; TouchCalibration calibration_; Preferences preferences_;
  bool nvs_ok_=false;
  void deselect(); bool start(uint32_t hz); void finish();
  bool command(uint8_t cmd,const uint8_t* data=nullptr,size_t count=0);
  bool initialize(uint32_t now); void flushChunk();
  bool readTouch(uint16_t& x,uint16_t& y);
  uint16_t touchSample(uint8_t cmd);
  void processTouch(PanelCore& core,uint32_t now);
  void paint(const Snapshot& s);
  void rectangle(int x,int y,int w,int h,uint16_t color);
  void text(int x,int y,const char* word,uint16_t color,int scale=1);
  void glyph(int x,int y,char ch,uint16_t color,int scale);
  void saveCalibration(); bool loadCalibration();
};
}
