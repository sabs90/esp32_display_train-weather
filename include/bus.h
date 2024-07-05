#ifndef __BUS_H__
#define __BUS_H__

#include <ArduinoJson.h>
#include <GxEPD2_GFX.h>

#include "app.h"
#include "renderer.h"

struct stopDescription {
  const char *name;
  std::vector<int> iconIds;
};

class Bus : public IApp {
 public:
  Bus(GxEPD2_GFX &display, Renderer &renderer);

  bool fetchData();
  void render();

 private:
  GxEPD2_GFX &_display;
  Renderer &_renderer;

  uint32_t batPercent;
  int wifiRSSI;  // “Received Signal Strength Indicator"
  time_t updateTime;

  std::vector<JsonDocument> stopDocs;

  bool fetchForStopId(const char *stopId, JsonDocument &stopDoc);
  void showBusStopDepartures(int16_t l, int16_t t, int16_t r, int16_t b);
  int16_t showDeparturesForStop(JsonDocument &stopDoc, int16_t l, int16_t t,
                                int16_t r, int16_t b);
  static std::vector<JsonObject> getSortedStopEvents(
      JsonArray stopEventsJsonArray);
  static stopDescription getStopDescription(JsonDocument &stopDoc);
  static const unsigned char *getBitmapForIconId(int16_t iconId);
  int16_t drawStopEvent(const JsonObject &stopEvent, int16_t l, int16_t t,
                        int16_t r, int16_t b);
  static time_t getDepartureTime(const JsonObject &stopEvent);
  static time_t parseTimeUtc(const char *utcTimeString);
};

#endif
