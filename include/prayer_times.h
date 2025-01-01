#ifndef PRAYER_TIMES_H
#define PRAYER_TIMES_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <GxEPD2_GFX.h>
#include "renderer.h"
#include "app.h"
#include <time.h>

class PrayerTimes : public IApp {
public:
    PrayerTimes(GxEPD2_GFX& display, Renderer& renderer);
    
    bool fetchData() override;
    void render() override;
    void setRenderArea(int16_t l, int16_t t, int16_t r, int16_t b);

private:
    GxEPD2_GFX& _display;
    Renderer& _renderer;
    int16_t _renderL, _renderT, _renderR, _renderB;

    struct Prayer {
        String name;
        time_t time;
    };

    Prayer prayers[6];
    int currentPrayerIndex;
    int nextPrayerIndex;
    time_t now;

    bool fetchPrayerTimes();
    void updateCurrentAndNextPrayer();
    String formatCountdown(time_t target);
    time_t parseTime(const String& timeStr);
};

#endif // PRAYER_TIMES_H