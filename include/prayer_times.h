#ifndef PRAYER_TIMES_H
#define PRAYER_TIMES_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <GxEPD2_GFX.h>
#include "renderer.h"
#include "app.h"
#include <time.h>
#include "display_modes.h"

class PrayerTimes : public IApp {
public:
    PrayerTimes(GxEPD2_GFX& display, Renderer& renderer);
    
    bool fetchData() override;
    //void render(DisplayMode mode = DisplayMode::NORMAL) override;
    void render() override;
    //void render(DisplayMode mode); 
    void setRenderArea(int16_t l, int16_t t, int16_t r, int16_t b);
    
    bool isNightTime() const;
    bool isWithinPrayerStart(int minutes) const;
    int getMinutesToNextPrayer() const;
    void renderModeSpecific(DisplayMode mode);

    void renderAlertMode();
    void renderNightMode();
    void renderLowPowerMode();
    void renderNormalMode();
    void renderCurrentPrayerMode();
    void renderPrayerMode();
    //void renderAlertMode();
    //void renderNightMode();
    //void renderLowPowerMode();

private:
    GxEPD2_GFX& _display;
    Renderer& _renderer;
    int16_t _renderL, _renderT, _renderR, _renderB;
    int16_t _renderWidth;
    int16_t _renderHeight;

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
    void renderProgressCircle(int16_t centerX, int16_t centerY, int16_t radius, float progress);
    void drawDomeOutline(int16_t x, int16_t y, int16_t w, int16_t h);
    void renderPrayerTimeline(int16_t x, int16_t y, int16_t w, int currentPrayer, bool invertColors = false);
    String formatTime(time_t t);
    time_t adjustToNextDay(time_t prayerTime);
};

#endif