#include "prayer_times.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <GxEPD2_GFX.h>

#include <StreamUtils.h> //what's this
#include <time.h> //what's this

#include "bus_icons.h"
#include "client_utils.h"
#include "renderer.h"
#include "secrets.h"
#include "prayer_icons.h"
#include "display_modes.h"
#include "mode_manager.h"
#include "quran_verses.h"
#include "night_bitmap.h"


PrayerTimes::PrayerTimes(GxEPD2_GFX& display, Renderer& renderer, SettingsServer& settingsServer)
    : _display(display), _renderer(renderer), _settingsServer(settingsServer), 
      _renderL(0), _renderT(0), _renderR(0), _renderB(0) {
    prayers[0] = {"Fajr", 0};
    prayers[1] = {"Sunrise", 0};
    prayers[2] = {"Dhuhr", 0};
    prayers[3] = {"Asr", 0};
    prayers[4] = {"Maghrib", 0};
    prayers[5] = {"Isha", 0};
}

bool PrayerTimes::fetchData() {
    return fetchPrayerTimes();
}

bool PrayerTimes::fetchPrayerTimes() {
    WiFiClientSecure client;
    HTTPClient http;

    client.setInsecure();

    // Get current settings
    const auto& settings = _settingsServer.getSettings();

    // Ensure we're using today's date in the API call
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char dateStr[11];
    strftime(dateStr, sizeof(dateStr), "%d-%m-%Y", &timeinfo);

    String url = "https://api.aladhan.com/v1/timings/" + String(dateStr);
    url += "?latitude=" + String(settings.latitude, 4);
    url += "&longitude=" + String(settings.longitude, 4);
    url += "&method=" + String(settings.calculationMethod);
    url += "&school=" + String(settings.school);
    url += "&adjustment=1";

    Serial.println("Requesting URL: " + url);
    
    if (!http.begin(client, url)) {
        Serial.println("Failed to begin HTTP client");
        return false;
    }

    http.addHeader("Accept", "application/json");
    
    int httpCode = http.GET();
    Serial.printf("HTTP GET response code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        // First, let's see the raw response
        Serial.println("Raw API response:");
        Serial.println(payload);

        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.print("JSON parsing failed: ");
            Serial.println(error.c_str());
            http.end();
            return false;
        }

        // Debug: Print the complete parsed JSON
        Serial.println("Parsed JSON:");
        serializeJsonPretty(doc, Serial);

        if (!doc.containsKey("data") || !doc["data"].containsKey("timings")) {
            Serial.println("Invalid API response format");
            http.end();
            return false;
        }
        
        // Extract Hijri date
        const char* hijriDay = doc["data"]["date"]["hijri"]["day"];
        const char* hijriMonth = doc["data"]["date"]["hijri"]["month"]["en"];
        hijriDate = String(hijriDay) + " " + String(hijriMonth);

        Serial.printf("Hijri date: %s\n", hijriDate.c_str());

        // Debug: Print current system time
        now = time(nullptr);
        char debugTimeStr[30];
        strftime(debugTimeStr, sizeof(debugTimeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        Serial.printf("Current system time: %s\n", debugTimeStr);

        const char* prayerNames[] = {"Fajr", "Sunrise", "Dhuhr", "Asr", "Maghrib", "Isha"};
        
        for (int i = 0; i < 6; i++) {
            String timeStr = doc["data"]["timings"][prayerNames[i]].as<String>();
            prayers[i].name = prayerNames[i];
            //prayers[i].time = parseTime(timeStr);
            
            // Special handling for Isha time if override is enabled
            if (prayers[i].name == "Isha") {
                prayers[i].time = getIshaTime(timeStr);
            } else {
                prayers[i].time = parseTime(timeStr);
            }

            // Debug: Print raw and parsed times
            char prayerTimeStr[30];
            strftime(prayerTimeStr, sizeof(prayerTimeStr), "%Y-%m-%d %H:%M:%S", localtime(&prayers[i].time));
            Serial.printf("Prayer: %s, Raw time: %s, Parsed time: %s\n", 
                         prayerNames[i], 
                         timeStr.c_str(),
                         prayerTimeStr);
        }

        updateCurrentAndNextPrayer();

        http.end();
        return true;
    } else {
        Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }
}

String PrayerTimes::formatTime(time_t t) {
    char timeStr[8];
    strftime(timeStr, sizeof(timeStr), "%I:%M%p", localtime(&t));
    
    // Convert to lowercase and remove leading zero
    String formatted = String(timeStr);
    if (formatted.startsWith("0")) {
        formatted = formatted.substring(1);
    }
    formatted.toLowerCase();
    return formatted;
}

void PrayerTimes::setRenderArea(int16_t l, int16_t t, int16_t r, int16_t b) {
    _renderL = l;
    _renderT = t;
    _renderR = r;
    _renderB = b;
    _renderWidth = r - l;  
    _renderHeight = b - t; 
}

void PrayerTimes::renderProgressCircle(int16_t centerX, int16_t centerY, int16_t radius, float progress) {
    // Circle parameters
    int16_t strokeWidth = 20;  // Width of the progress arc
    float startAngle = -90;    // Start at top (-90 degrees)
    float totalAngle = 360.0f;
    float progressAngle = startAngle + (progress * totalAngle);
    
    // First, draw the background circle in light gray
    for (int16_t r = radius - strokeWidth + 1; r <= radius; r++) {
        float angle;
        for (angle = startAngle; angle <= startAngle + totalAngle; angle += 0.1) {
            float radAngle = (angle * PI) / 180.0f;
            int16_t x = centerX + (r * cos(radAngle));
            int16_t y = centerY + (r * sin(radAngle));
            _display.drawPixel(x, y, GxEPD_LIGHTGREY);  // Changed to LIGHTGREY
        }
    }
    
    // Then overlay the progress in black
    for (int16_t r = radius - strokeWidth + 1; r <= radius; r++) {
        float angle;
        for (angle = startAngle; angle <= progressAngle; angle += 0.1) {
            float radAngle = (angle * PI) / 180.0f;
            int16_t x = centerX + (r * cos(radAngle));
            int16_t y = centerY + (r * sin(radAngle));
            _display.drawPixel(x, y, GxEPD_BLACK);
        }
    }
}

void PrayerTimes::render() {
    static DisplayMode lastMode = DisplayMode::NORMAL;
    static time_t lastUpdate = 0;
    
    // Only update time and prayers if needed
    time_t currentTime = time(nullptr);
    if (currentTime - lastUpdate >= 1) { // Update at most once per second
        now = currentTime;
        updateCurrentAndNextPrayer();
        lastUpdate = currentTime;
    }

    // Get current display mode from ModeManager
    DisplayMode currentMode = ModeManager::determineMode(0, *this);

    // Call appropriate render function based on mode
    switch (currentMode) {
        case DisplayMode::LOW_POWER:
            renderLowPowerMode();
            break;
        case DisplayMode::ALERT:
            renderAlertMode();
            break;
        case DisplayMode::NIGHT:
            renderNightMode();
            break;
        case DisplayMode::CURRENT_PRAYER:
        case DisplayMode::NORMAL:
            renderPrayerMode();
            break;
        default:
            renderPrayerMode();
            break;
    }
}

void PrayerTimes::renderPrayerMode() {
    if (currentPrayerIndex < 0) return;

    // Get current mode to determine rendering style
    DisplayMode currentMode = ModeManager::determineMode(0, *this);
    bool isCurrentPrayerMode = (currentMode == DisplayMode::CURRENT_PRAYER);

    // Initialize base font and layout parameters
    _display.setFont(&FreeSansBold9pt7b);

    // Calculate layout parameters
    int16_t centerX = _display.width() / 2;
    int16_t centerY = _display.height() / 4 + 50;
    int16_t countdownY = isCurrentPrayerMode ? _renderT + 120 : centerY - 20;

    renderPrayerTimeline(_renderL, _renderT, _renderR - _renderL, currentPrayerIndex);

    // Normal mode shows progress circle, current prayer mode doesn't
    if (!isCurrentPrayerMode) {
        // Calculate and render progress circle
        time_t nextPrayerTime = prayers[nextPrayerIndex].time;
        time_t prevPrayerTime = prayers[currentPrayerIndex].time;
        time_t totalInterval = nextPrayerTime - prevPrayerTime;
        time_t elapsed = now - prevPrayerTime;
        float progress = 1.0f - (float)elapsed / totalInterval;

        int16_t circleRadius = 150;
        renderProgressCircle(centerX, centerY, circleRadius, progress);
    }

    // Render countdown and prayer info with mode-specific styling
    _display.setTextColor(GxEPD_BLACK);
    if (isCurrentPrayerMode) {
        // Current prayer mode: larger text, different layout
        _display.setFont(&FreeSansBold24pt7b);
        _renderer.drawString(centerX, centerY - 20,
                           prayers[currentPrayerIndex].name, CENTER);
        
        _display.setFont(&FreeSansBold18pt7b);
        _renderer.drawString(centerX, centerY + 40, "Started at", CENTER);
        String startTime = formatTime(prayers[currentPrayerIndex].time);
        _display.setFont(&FreeSansBold24pt7b);
        _renderer.drawString(centerX, centerY + 100, startTime, CENTER);

    } else {
        // Normal mode: regular countdown display
        _display.setFont(&FreeSansBold18pt7b);
        _renderer.drawString(centerX, centerY - 20, 
                           formatCountdown(prayers[nextPrayerIndex].time), CENTER);
        _display.setFont(&FreeSans9pt7b);


        // Special handling for Sunrise
        if (prayers[currentPrayerIndex].name == "Sunrise") {
            _renderer.drawString(centerX, centerY + 10, "until", CENTER);
            _display.setFont(&FreeSansBold18pt7b);
            _renderer.drawString(centerX, centerY + 50, 
                            prayers[nextPrayerIndex].name, CENTER);
        } else {
            _renderer.drawString(centerX, centerY + 10, "left for", CENTER);
            _display.setFont(&FreeSansBold18pt7b);
            _renderer.drawString(centerX, centerY + 50, 
                            prayers[currentPrayerIndex].name, CENTER);
        }
                           
    }

   // Hijri calendar
    _display.setFont(&FreeSansBold18pt7b);
    _renderer.drawString(_display.width() - X_MARGIN, Y_MARGIN + 20, 
                        hijriDate, RIGHT);

    // Add daily verse (common to both modes)
    renderDailyVerse();
}

void PrayerTimes::renderAlertMode() {
    // Fill entire screen with black background
    _display.fillScreen(GxEPD_BLACK);
    _display.setTextColor(GxEPD_WHITE);  // Set text color to white

    // Update current time
    now = time(nullptr);
    updateCurrentAndNextPrayer();

    // Get minutes until next prayer
    int minutesLeft = getMinutesToNextPrayer();
    
    // Draw large countdown
    _display.setFont(&FreeSansBold24pt7b);
    String countText = String(minutesLeft) + " MINUTES";

    uint32_t topborder = _display.height() / 4 - 25;

    //_renderer.drawString(_renderL + _renderWidth/2, _renderT + 100, countText, CENTER);
    _renderer.drawString(_renderL + _renderWidth/2,  topborder, countText, CENTER);
    
    _display.setFont(&FreeSansBold18pt7b);
    _renderer.drawString(_renderL + _renderWidth/2, topborder + 60, "LEFT FOR", CENTER);
    
    _display.setFont(&FreeSansBold24pt7b);
    _renderer.drawString(_renderL + _renderWidth/2, topborder + 120, 
                        prayers[currentPrayerIndex].name, CENTER);
    
    _display.setFont(&FreeSansBold18pt7b);
    _renderer.drawString(_renderL + _renderWidth/2, topborder + 180, 
                        formatTime(prayers[nextPrayerIndex].time), CENTER);

    // Render prayer timeline
    _display.setFont(&FreeSansBold9pt7b);
    renderPrayerTimeline(_renderL, _renderT, _renderR - _renderL, currentPrayerIndex, true);


    // Add verse display with inverted colors
    renderDailyVerse(true);
}

void PrayerTimes::renderNightMode() {
    // Fill entire screen with black background
    //_display.fillRect(_renderL, _renderT, _renderR - _renderL, _renderB - _renderT, GxEPD_BLACK);
    _display.fillScreen(GxEPD_BLACK);
    _display.setTextColor(GxEPD_WHITE);  // Set text color to white
    
    // Format current date and times
    char dateStr[32];
    strftime(dateStr, sizeof(dateStr), "%d %b %y", localtime(&now));
    
    // Draw "Isha started at" message with proper centering
    String ishaTime = formatTime(prayers[5].time);
    _display.setFont(&FreeSansBold24pt7b);
    //String ishaMessage = "Isha started at " + ishaTime;
    String ishaMessage = ishaTime;
    _renderer.drawString(_renderL + (_renderR - _renderL)/2, _display.height()/4 , "Isha started at ", CENTER);
    _renderer.drawString(_renderL + (_renderR - _renderL)/2, _display.height()/4 + 50, ishaMessage, CENTER);
    
    // Calculate center position for the bitmap
    int16_t bitmapX = (_display.width() - NIGHT_BITMAP_WIDTH) / 2;
    int16_t bitmapY = (_display.height() - NIGHT_BITMAP_HEIGHT) / 2 - 50;
    
    // Draw the bitmap in white on black background
    _display.drawBitmap(
        bitmapX, 
        bitmapY,
        night_bitmap,
        NIGHT_BITMAP_WIDTH,
        NIGHT_BITMAP_HEIGHT,
        GxEPD_WHITE  // Foreground color
    );
    

    // Draw next prayer info at bottom
    _display.setFont(&FreeSansBold18pt7b);
    String nextPrayersInfo = "Fajr at " + formatTime(prayers[0].time) + 
                           ", Sunrise " + formatTime(prayers[1].time);
    _renderer.drawString(_renderL + (_renderR - _renderL)/2, _display.height() - Y_MARGIN - 100, "Fajr: " + formatTime(prayers[0].time), CENTER);
    _renderer.drawString(_renderL + (_renderR - _renderL)/2, _display.height() - Y_MARGIN - 60, "Sunrise: " + formatTime(prayers[1].time), CENTER);
}

void PrayerTimes::renderLowPowerMode() {
    // Simple black and white display showing just prayer times
    render(); // For now, just use normal render
}

time_t PrayerTimes::adjustToNextDay(time_t prayerTime) {
    // Helper function to adjust prayer time to next day if needed
    struct tm timeinfo;
    localtime_r(&prayerTime, &timeinfo);
    timeinfo.tm_mday++;  // Add one day
    timeinfo.tm_hour = timeinfo.tm_hour;  // Preserve hour
    timeinfo.tm_min = timeinfo.tm_min;    // Preserve minute
    return mktime(&timeinfo);
}

void PrayerTimes::updateCurrentAndNextPrayer() {
    now = time(nullptr);
    char currentTimeStr[30];
    strftime(currentTimeStr, sizeof(currentTimeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    Serial.printf("\nUpdating prayers at: %s\n", currentTimeStr);

    // If we're after Isha (21:46) but before midnight
    if (now >= prayers[5].time) {
        currentPrayerIndex = 5;  // Current is Isha
        nextPrayerIndex = 0;     // Next is tomorrow's Fajr
        
        // Get current date at midnight
        struct tm todayMidnight;
        localtime_r(&now, &todayMidnight);
        todayMidnight.tm_hour = 0;
        todayMidnight.tm_min = 0;
        todayMidnight.tm_sec = 0;
        todayMidnight.tm_mday++; // Add one day
        
        // Adjust Fajr time to tomorrow
        struct tm fajrTime;
        localtime_r(&prayers[0].time, &fajrTime);
        
        // Set to tomorrow's date but keep Fajr's time
        fajrTime.tm_mday = todayMidnight.tm_mday;
        fajrTime.tm_mon = todayMidnight.tm_mon;
        fajrTime.tm_year = todayMidnight.tm_year;
        
        prayers[0].time = mktime(&fajrTime);
        
        char debugStr[30];
        strftime(debugStr, sizeof(debugStr), "%Y-%m-%d %H:%M:%S", &fajrTime);
        Serial.printf("Adjusted tomorrow's Fajr to: %s\n", debugStr);
        
        return;
    }

    // Normal daytime processing - find the next prayer
    for (int i = 0; i < 6; i++) {
        if (now < prayers[i].time) {
            nextPrayerIndex = i;
            currentPrayerIndex = (i > 0) ? i - 1 : 5;
            
            // Debug output
            char nextTime[30];
            strftime(nextTime, sizeof(nextTime), "%Y-%m-%d %H:%M:%S", localtime(&prayers[nextPrayerIndex].time));
            Serial.printf("Current: %s, Next: %s at %s\n", 
                prayers[currentPrayerIndex].name,
                prayers[nextPrayerIndex].name,
                nextTime);
                
            return;
        }
    }
}

String PrayerTimes::formatCountdown(time_t target) {
    time_t diff = target - now;
    int hours = diff / 3600;
    int minutes = (diff % 3600) / 60;
    
    String result;
    
    // Add hours part if there are hours
    if (hours > 0) {
        result += String(hours);
        result += (hours == 1) ? " hr" : " hrs";
        if (minutes > 0) result += " ";
    }
    
    // Add minutes part if there are minutes or no hours
    if (minutes > 0 || hours == 0) {
        result += String(minutes);
        result += (minutes == 1) ? " min" : " mins";
    }
    
    return result;
}

time_t PrayerTimes::parseTime(const String& timeStr) {
    Serial.printf("Parsing time string: %s\n", timeStr.c_str());
    
    struct tm timeinfo;
    int hour, minute;
    
    // Clear the structure
    memset(&timeinfo, 0, sizeof(struct tm));
    
    // Parse the time string and check for errors
    if (sscanf(timeStr.c_str(), "%d:%d", &hour, &minute) != 2) {
        Serial.printf("Failed to parse time string: %s\n", timeStr.c_str());
        return 0;
    }
    
    Serial.printf("Parsed hours: %d, minutes: %d\n", hour, minute);
    
    // Get current time for date information
    time_t currentTime = time(nullptr);
    localtime_r(&currentTime, &timeinfo);
    
    // Set the parsed time for today
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = 0;
    
    // Convert to time_t
    time_t result = mktime(&timeinfo);
    
    // Debug output
    char buffer[26];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.printf("Parsed time: %s\n", buffer);
    
    return result;
}

bool PrayerTimes::isWithinPrayerStart(int minutes) const {
    time_t now = time(NULL);
    if (currentPrayerIndex < 0) return false;
    
    time_t prayerStart = prayers[currentPrayerIndex].time;
    time_t timeSincePrayer = now - prayerStart;
    return timeSincePrayer >= 0 && timeSincePrayer <= (minutes * 60);
}

void PrayerTimes::drawDomeOutline(int16_t x, int16_t y, int16_t w, int16_t h) {
    // Draw dome curve
    int16_t domeHeight = h * 0.2; // 20% of total height for dome
    int16_t domeWidth = w * 0.8;  // 80% of total width
    int16_t startX = x + (w - domeWidth)/2;
    
    // Draw the dome curve using multiple small arcs
    for (int16_t i = 0; i < domeWidth/2; i++) {
        float angle = i * PI / domeWidth;
        int16_t y1 = y + domeHeight - (sin(angle) * domeHeight);
        int16_t y2 = y + domeHeight - (sin(angle + 0.1) * domeHeight);
        _display.drawLine(startX + i, y1, startX + i + 1, y2, GxEPD_BLACK);
        _display.drawLine(startX + domeWidth - i, y1, 
                         startX + domeWidth - i - 1, y2, GxEPD_BLACK);
    }

    // Draw the vertical sides
    _display.drawFastVLine(startX, y + domeHeight, h/4, GxEPD_BLACK);
    _display.drawFastVLine(startX + domeWidth, y + domeHeight, h/4, GxEPD_BLACK);
}

void PrayerTimes::renderPrayerTimeline(int16_t x, int16_t y, int16_t w, int currentPrayer, bool invertColors) {
    // Constants for layout
    const int16_t VERTICAL_OFFSET = 30;
    const int16_t NAME_MARGIN = 10;
    const int16_t ICON_SPACING = 20;
    const int16_t TIME_SPACING = 50;
    const int16_t HIGHLIGHT_PADDING = 10;
    const int16_t HIGHLIGHT_RADIUS = 10;
    const int16_t OUTLINE_THICKNESS = 2;
    
    // Calculate positions
    y += VERTICAL_OFFSET;
    int16_t yPrayerNames = y + NAME_MARGIN;
    int16_t yPrayerIcons = yPrayerNames + ICON_SPACING;  // Position icons below names
    int16_t yPrayerTimes = yPrayerIcons + PRAYER_ICON_HEIGHT + 20;  // Position times below icons
    int16_t xInterval = w / 6;
    int16_t boxheight = yPrayerTimes - y + 20;  // Increased height to accommodate icons

    // Render each prayer slot
    for (int i = 0; i < 6; i++) {
        bool isHighlighted = (i == currentPrayer);
        int16_t xCenter = x + (xInterval * i) + (xInterval / 2);
        int16_t xLeft = x + (xInterval * i);

        // Handle background and highlighting
        if (invertColors) {
            // Dark mode: all prayers have dark background with white text
            _display.fillRoundRect(
                xLeft, 
                y - HIGHLIGHT_PADDING, 
                xInterval, 
                boxheight + (2 * HIGHLIGHT_PADDING), 
                HIGHLIGHT_RADIUS, 
                GxEPD_BLACK
            );
            
            // Current prayer gets white outline
            if (isHighlighted) {
                for (int16_t t = 0; t < OUTLINE_THICKNESS; t++) {
                    _display.drawRoundRect(
                        xLeft + t, 
                        y - HIGHLIGHT_PADDING + t, 
                        xInterval - (2 * t), 
                        boxheight + (2 * HIGHLIGHT_PADDING) - (2 * t), 
                        HIGHLIGHT_RADIUS,
                        GxEPD_WHITE
                    );
                }
            }
            _display.setTextColor(GxEPD_WHITE);
        } else {
            // Light mode: highlight current prayer with black background
            if (isHighlighted) {
                _display.fillRoundRect(
                    xLeft, 
                    y - HIGHLIGHT_PADDING, 
                    xInterval, 
                    //boxheight + (2 * HIGHLIGHT_PADDING),
                    boxheight + 5, 
                    HIGHLIGHT_RADIUS, 
                    GxEPD_BLACK
                );
                _display.setTextColor(GxEPD_WHITE);
            } else {
                _display.setTextColor(GxEPD_BLACK);
            }
        }

        // Draw prayer name
        _renderer.drawString(xCenter, yPrayerNames, prayers[i].name, CENTER);

        // Draw the prayer icon
        const unsigned char* icon = getPrayerIcon(prayers[i].name);

        _display.drawBitmap(
            xCenter - (PRAYER_ICON_WIDTH / 2),  // Center the icon horizontally
            yPrayerIcons,                       // Position icon below name
            icon,
            PRAYER_ICON_WIDTH,
            PRAYER_ICON_HEIGHT,
            (invertColors || isHighlighted) ? GxEPD_WHITE : GxEPD_BLACK
        );

        // Draw prayer time
        //String formattedTime = formatTime(prayers[i].time);
        //_renderer.drawString(xCenter, yPrayerTimes, formattedTime, CENTER);
        String formattedTime = formatTimelineTime(prayers[i].time);
        _renderer.drawString(xCenter, yPrayerTimes, formattedTime, CENTER);
    }

    // Reset text color to default
    _display.setTextColor(GxEPD_BLACK);
}

bool PrayerTimes::isNightTime() const {
    time_t now = time(NULL);
    
    // Debug logging
    char timeStr[30];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    Serial.printf("\nChecking night time at: %s\n", timeStr);

    // Log prayer times
    char fajrStr[30], ishaStr[30];
    strftime(fajrStr, sizeof(fajrStr), "%I:%M%p", localtime(&prayers[0].time));
    strftime(ishaStr, sizeof(ishaStr), "%I:%M%p", localtime(&prayers[5].time));
    Serial.printf("  Fajr time: %s\n", fajrStr);
    Serial.printf("  Isha time: %s\n", ishaStr);

    // If we're after Isha but before midnight
    if (now >= prayers[5].time) {
        Serial.println("  Night time: Yes (after Isha)");
        return true;
    }
    
    // If we're after midnight but before Fajr begins
    if (now < prayers[0].time) {
        Serial.println("  Night time: Yes (before Fajr)");
        return true;
    }
    
    Serial.println("  Night time: No (daytime or during Fajr)");
    return false;
}

int PrayerTimes::getMinutesToNextPrayer() const {
    if (nextPrayerIndex < 0) return INT_MAX;
    
    time_t now = time(NULL);
    return (prayers[nextPrayerIndex].time - now) / 60;
}

void PrayerTimes::updateDailyVerse() {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // Check if we need to update the verse (new day or first run)
    if (lastVerseUpdate == 0 || timeinfo.tm_mday != localtime(&lastVerseUpdate)->tm_mday) {
        // Simple rotation through verses based on the day of the year
        currentVerseIndex = timeinfo.tm_yday % NUM_VERSES;
        lastVerseUpdate = now;
        Serial.printf("Updated daily verse to index %d\n", currentVerseIndex);
    }
}

void PrayerTimes::renderDailyVerse(bool invertColors) {
    updateDailyVerse();
    
    // Calculate position for verse (in the bottom quarter, above status bar)
    //int16_t verseY = _renderB - 80;  // Adjust this value as needed
    int16_t verseY = _display.height() - (_display.height()/4) + 30;
    int16_t maxWidth = _renderR - _renderL - 40;  // Leave margins
    
    // Set font and color
    _display.setFont(&FreeSans9pt7b);
    _display.setTextColor(invertColors ? GxEPD_WHITE : GxEPD_BLACK);
    
    // Draw the verse text
    const QuranVerse& verse = DAILY_VERSES[currentVerseIndex];
    
    // Draw translation with multi-line support
    _renderer.drawMultiLnString(_renderL + (_renderR - _renderL)/2, 
                               verseY,
                               String(verse.translation),
                               CENTER,
                               maxWidth,
                               2,  // max 2 lines
                               20); // line spacing
    
    // Draw reference below
    _display.setFont(&FreeSans9pt7b);
    _renderer.drawString(_renderL + (_renderR - _renderL)/2,
                        verseY + 50,
                        String("— ") + verse.reference,
                        CENTER);
}

String PrayerTimes::formatTimelineTime(time_t t) {
    char timeStr[6];
    strftime(timeStr, sizeof(timeStr), "%I:%M", localtime(&t));
    
    // Convert to lowercase and remove leading zero
    String formatted = String(timeStr);
    if (formatted.startsWith("0")) {
        formatted = formatted.substring(1);
    }
    return formatted;
}


