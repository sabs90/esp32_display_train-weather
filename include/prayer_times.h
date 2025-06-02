#ifndef PRAYER_TIMES_H
#define PRAYER_TIMES_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <GxEPD2_GFX.h>
#include "renderer.h"
#include "app.h"
#include "settings_server.h"
#include <time.h>
#include "display_modes.h"

#define NIGHT_BITMAP_WIDTH 512
#define NIGHT_BITMAP_HEIGHT 512
extern const unsigned char night_bitmap[] PROGMEM;
class PrayerTimes : public IApp {
public:
    //PrayerTimes(GxEPD2_GFX& display, Renderer& renderer);
    PrayerTimes(GxEPD2_GFX& display, Renderer& renderer, SettingsServer& settingsServer);
    
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

    int getCurrentPrayerIndex() const { return currentPrayerIndex; }
    int getNextPrayerIndex() const { return nextPrayerIndex; }


private:
    GxEPD2_GFX& _display;
    Renderer& _renderer;
    SettingsServer& _settingsServer;
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

    time_t lastVerseUpdate = 0;
    int currentVerseIndex = 0;
    void updateDailyVerse();
    void renderDailyVerse(bool invertColors = false);
    String formatTimelineTime(time_t t);

    // Add islamic calendar
    String hijriDate;


    // Isha override settings
    static constexpr bool ISHA_OVERRIDE_ENABLED = false;  // Set to true to enable override
    static constexpr int ISHA_OVERRIDE_HOUR = 21;        // 24-hour format (e.g., 19 for 7 PM)
    static constexpr int ISHA_OVERRIDE_MINUTE = 30;      // Minutes (e.g., 30 for 7:30 PM)

    // Helper function to get Isha time considering override
    time_t getIshaTime(const String& apiTimeStr) {
        if (!ISHA_OVERRIDE_ENABLED) {
            return parseTime(apiTimeStr);
        }

        // Create override time using current date
        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        
        // Set the override hour and minute while keeping today's date
        timeinfo.tm_hour = ISHA_OVERRIDE_HOUR;
        timeinfo.tm_min = ISHA_OVERRIDE_MINUTE;
        timeinfo.tm_sec = 0;
        
        time_t overrideTime = mktime(&timeinfo);
        
        // Debug output
        char debugTime[30];
        strftime(debugTime, sizeof(debugTime), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("Using Isha override time: %s\n", debugTime);
        
        return overrideTime;
    }
};

#endif