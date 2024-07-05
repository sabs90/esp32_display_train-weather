#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <Arduino.h>
#include <GxEPD2_GFX.h>
#include <time.h>

#include <vector>

#include "config.h"

typedef enum alignment { LEFT, RIGHT, CENTER } alignment_t;

class Renderer {
 public:
  Renderer(GxEPD2_GFX &display);

  uint16_t getStringWidth(const String &text);
  uint16_t getStringHeight(const String &text);
  void drawString(int16_t x, int16_t y, const String &text,
                  alignment_t alignment);
  int16_t drawMultiLnString(int16_t x, int16_t y, const String &text,
                            alignment_t alignment, uint16_t max_width,
                            uint16_t max_lines, int16_t line_spacing);

  void drawArc(int16_t cx, int16_t cy, double startAngle, double endAngle,
               int16_t innerRadius, int16_t outerRadius);

  int16_t drawStatusBar(int16_t yBaseline, time_t lastUpdatedTime, int rssi,
                        uint32_t batPercent);
  void drawError(const uint8_t *bitmap_196x196, const String &errMsgLn1,
                 const String &errMsgLn2 = "");

 private:
  GxEPD2_GFX &_display;

  void drawBattery(int16_t x, int16_t y, int16_t w, int16_t h,
                   uint32_t batPercent);
  void drawWifi(int16_t x, int16_t y, int16_t w, int16_t h, int rssi);
  static boolean isAngleInArc(double angle, double startAngle, double endAngle);
};

#endif
