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


PrayerTimes::PrayerTimes(GxEPD2_GFX& display, Renderer& renderer)
    : _display(display), _renderer(renderer), _renderL(0), _renderT(0), _renderR(0), _renderB(0) {
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

    // Ensure we're using today's date in the API call
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char dateStr[11];
    strftime(dateStr, sizeof(dateStr), "%d-%m-%Y", &timeinfo);

    String url = "https://api.aladhan.com/v1/timings/" + String(dateStr);
    url += "?latitude=-33.8688";
    url += "&longitude=151.2093";
    url += "&method=3";
    url += "&school=1";
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

        // Debug: Print current system time
        now = time(nullptr);
        char debugTimeStr[30];
        strftime(debugTimeStr, sizeof(debugTimeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        Serial.printf("Current system time: %s\n", debugTimeStr);

        const char* prayerNames[] = {"Fajr", "Sunrise", "Dhuhr", "Asr", "Maghrib", "Isha"};
        
        for (int i = 0; i < 6; i++) {
            String timeStr = doc["data"]["timings"][prayerNames[i]].as<String>();
            prayers[i].name = prayerNames[i];
            prayers[i].time = parseTime(timeStr);
            
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
/*
void PrayerTimes::renderModeSpecific(DisplayMode mode) {
    switch (mode) {
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
            renderCurrentPrayerMode();
            break;
        default:
            render(); // Normal render
            break;
    }
}
*/

void PrayerTimes::renderModeSpecific(DisplayMode mode) {
    switch (mode) {
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
        default:
            render(); // Handle both normal and current prayer modes
            break;
    }
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

/* DELETE
void PrayerTimes::render() {
    // Update current time
    now = time(nullptr);
    updateCurrentAndNextPrayer();

    _display.setFont(&FreeSansBold9pt7b);
    
    // set your heights
    int16_t yPrayerNames = _renderT + 10;
    int16_t yPrayerIcons = yPrayerNames + 30;
    int16_t yPrayerTimes = yPrayerIcons + 50;

    // x-interval
    int16_t xInterval = (_renderR - _renderL) / 6; //make this dynamic re: #prayer times we're looking at. Although prob not needed. 
    int16_t xMarginPrayerRender = 3;

    int16_t y = _renderT + 30; //delete this crap
    int16_t lineHeight = 35; // delete this crap
    int16_t boxheight = yPrayerTimes - _renderT;

    for (int i = 0; i < 6; i++) {
        if (i == currentPrayerIndex) {
            _display.setTextColor(GxEPD_WHITE);
            _display.fillRoundRect(_renderL + i * xInterval, _renderT - 10, xInterval, boxheight + 20, 10, GxEPD_BLACK);
            
        } else {
            _display.setTextColor(GxEPD_BLACK);
        }

        /*
        char timeStr[6];
        strftime(timeStr, sizeof(timeStr), "%H:%M", localtime(&prayers[i].time));
        */
        /* DELETE
        char timeStr[8];  // Increased size to accommodate AM/PM
        strftime(timeStr, sizeof(timeStr), "%I:%M%p", localtime(&prayers[i].time));

        // Convert AM/PM to lowercase and remove leading zero from hour
        String formattedTime = String(timeStr);
        if (formattedTime.startsWith("0")) {
            formattedTime = formattedTime.substring(1);  // Remove leading zero
        }
        formattedTime.toLowerCase();  // Convert am/pm to lowercase

        _renderer.drawString(_renderL + xInterval * i + xInterval/2, yPrayerNames, prayers[i].name, CENTER);
        _renderer.drawString(_renderL + xInterval * i + xInterval/2, yPrayerTimes, formattedTime, CENTER);

        /*
         _renderer.drawString(_renderL + xInterval * i + xInterval/2, yPrayerNames, prayers[i].name, CENTER);
         _renderer.drawString(_renderL + xInterval * i + xInterval/2, yPrayerTimes, String(timeStr), CENTER);
         _display.drawInvertedBitmap(_renderL + xInterval * i + xInterval/2 , yPrayerIcons,  epd_bitmap_busiconplaceholder , 32, 32, GxEPD_WHITE);
        */
         //render prayer time icon!
        // Draw the prayer icon
        /*
        const unsigned char* icon = getPrayerIcon(prayers[i].name);
        _display.drawInvertedBitmap(
            _renderL + xInterval * i + xInterval/2 - PRAYER_ICON_WIDTH/2,
            yPrayerIcons,
            icon,
            PRAYER_ICON_WIDTH,
            PRAYER_ICON_HEIGHT,
            i == currentPrayerIndex ? GxEPD_WHITE : GxEPD_BLACK
        );
        */
        /* DELETE
        y += lineHeight;
    }*/

/*
void PrayerTimes::render() {
    render(DisplayMode::NORMAL);  // Default implementation calls the mode-specific version
    
}
*/

// =====================================================

// In prayer_times.cpp, modify the render() function:
/*v2
void PrayerTimes::render() {
    // Update current time
    now = time(nullptr);
    updateCurrentAndNextPrayer();

    // Get current display mode from ModeManager
    DisplayMode currentMode = ModeManager::determineMode(0, *this); // Pass 0 as battery since we don't need it here

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
            //renderCurrentPrayerMode();
            renderPrayerMode();
            break;
        //case DisplayMode::NORMAL:
        default:
            //renderNormalMode();
            renderPrayerMode();
            break;
    }
}
*/

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

    // Render prayer timeline with appropriate highlight
    renderPrayerTimeline(_renderL, _renderT, _renderR - _renderL, 
        isCurrentPrayerMode ? nextPrayerIndex : currentPrayerIndex);

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
        //String startTime = "Started at " + formatTime(prayers[currentPrayerIndex].time);
        String startTime = formatTime(prayers[currentPrayerIndex].time);
        _display.setFont(&FreeSansBold24pt7b);
        _renderer.drawString(centerX, centerY + 100, startTime, CENTER);

    } else {
        // Normal mode: regular countdown display
        _display.setFont(&FreeSansBold18pt7b);
        _renderer.drawString(centerX, centerY - 20, 
                           formatCountdown(prayers[nextPrayerIndex].time), CENTER);
        _display.setFont(&FreeSans9pt7b);
        _renderer.drawString(centerX, centerY + 10, "left for", CENTER);
        _display.setFont(&FreeSansBold18pt7b);
        _renderer.drawString(centerX, centerY + 50, 
                           prayers[currentPrayerIndex].name, CENTER);
    }

    // Add current time (common to both modes)
    String formattedTime = formatTime(now);
    _display.setFont(&FreeSansBold18pt7b);
    _renderer.drawString(_display.width() - X_MARGIN, Y_MARGIN + 20, 
                        formattedTime, RIGHT);
}

// Add new function for normal mode rendering (extracted from existing render code)
/* v6
void PrayerTimes::renderNormalMode() {
    // Initialize base font and layout parameters
    _display.setFont(&FreeSansBold9pt7b);

    // Render prayer timeline
    renderPrayerTimeline(_renderL, _renderT, _renderR - _renderL, currentPrayerIndex);

    // Calculate progress for the circle
    time_t nextPrayerTime = prayers[nextPrayerIndex].time;
    time_t prevPrayerTime = prayers[currentPrayerIndex].time;
    time_t totalInterval = nextPrayerTime - prevPrayerTime;
    time_t elapsed = now - prevPrayerTime;
    float progress = 1.0f - (float)elapsed / totalInterval;

    // Draw the progress circle
    int16_t circleRadius = 150;
    int16_t circleCenterX = _display.width() / 2;
    int16_t circleCenterY = _display.height() / 4 + 50;
    renderProgressCircle(circleCenterX, circleCenterY, circleRadius, progress);

    // Render countdown text
    _display.setFont(&FreeSansBold18pt7b);
    _display.setTextColor(GxEPD_BLACK);
    _renderer.drawString(circleCenterX, circleCenterY - 20, 
                        formatCountdown(prayers[nextPrayerIndex].time), CENTER);
    _display.setFont(&FreeSans9pt7b);
    _renderer.drawString(circleCenterX, circleCenterY + 10, "left for", CENTER);
    _display.setFont(&FreeSansBold18pt7b);
    _renderer.drawString(circleCenterX, circleCenterY + 50, 
                        prayers[currentPrayerIndex].name, CENTER);

    // Add current time
    String formattedTime = formatTime(now);
    _display.setFont(&FreeSansBold18pt7b);
    _display.setTextColor(GxEPD_BLACK);
    _renderer.drawString(_display.width() - X_MARGIN, Y_MARGIN + 20, 
                        formattedTime, RIGHT);
}
*/

// Add new function for current prayer mode rendering
void PrayerTimes::renderCurrentPrayerMode() {
    if (currentPrayerIndex < 0) return;

    // Draw dome outline
    drawDomeOutline(_renderL, _renderT, _renderWidth, _renderHeight);

    // Draw prayer timeline with next prayer highlighted
    renderPrayerTimeline(_renderL, _renderT, _renderR - _renderL, nextPrayerIndex);

    // Draw large text with current prayer info
    _display.setFont(&FreeSansBold24pt7b);
    _display.setTextColor(GxEPD_BLACK);
    _renderer.drawString(_renderL + (_renderR - _renderL)/2, 
                        _renderT + 120, prayers[currentPrayerIndex].name, CENTER);
    
    _display.setFont(&FreeSansBold18pt7b);
    String startTime = "Started at " + formatTime(prayers[currentPrayerIndex].time);
    _renderer.drawString(_renderL + (_renderR - _renderL)/2, 
                        _renderT + 170, startTime, CENTER);
    
    // Add current time
    String formattedTime = formatTime(now);
    _display.setFont(&FreeSansBold18pt7b);
    _display.setTextColor(GxEPD_BLACK);
    _renderer.drawString(_display.width() - X_MARGIN, Y_MARGIN + 20, 
                        formattedTime, RIGHT);
}

// =====================================================

/* v5
void PrayerTimes::render(DisplayMode mode) {
    // Update current time
    now = time(nullptr);
    updateCurrentAndNextPrayer();

    // Initialize base font and layout parameters
    _display.setFont(&FreeSansBold9pt7b);

    // Render prayer timeline
    renderPrayerTimeline(_renderL, _renderT, _renderR - _renderL, 
        (mode == DisplayMode::CURRENT_PRAYER) ? nextPrayerIndex : currentPrayerIndex);
    /*
    // Set heights for prayer display
    int16_t yPrayerNames = _renderT + 10;
    int16_t yPrayerIcons = yPrayerNames + 30;
    int16_t yPrayerTimes = yPrayerIcons + 50;
    int16_t xInterval = (_renderR - _renderL) / 6;
    int16_t boxheight = yPrayerTimes - _renderT;
    
    DisplayMode currentMode = DisplayMode::NORMAL; // This should be passed in as a parameter
    //int prayerToHighlight = isCurrentPrayerMode ? nextPrayerIndex : currentPrayerIndex;
    int prayerToHighlight = (mode == DisplayMode::CURRENT_PRAYER) ? nextPrayerIndex : currentPrayerIndex;

    // Render prayer timeline
    for (int i = 0; i < 6; i++) {
        if (i == prayerToHighlight) {
            _display.setTextColor(GxEPD_WHITE);
            _display.fillRoundRect(_renderL + i * xInterval, _renderT - 10, 
                                 xInterval, boxheight + 20, 10, GxEPD_BLACK);
        } else {
            _display.setTextColor(GxEPD_BLACK);
        }

        // Format time string
        String formattedTime = formatTime(prayers[i].time);

        // Use larger font for current prayer mode
        if (mode == DisplayMode::CURRENT_PRAYER) {
            _display.setFont(&FreeSansBold12pt7b);
        }

        _renderer.drawString(_renderL + xInterval * i + xInterval/2, 
                           yPrayerNames, prayers[i].name, CENTER);
        _renderer.drawString(_renderL + xInterval * i + xInterval/2, 
                           yPrayerTimes, formattedTime, CENTER);

        // Reset to normal font size
        if (mode == DisplayMode::CURRENT_PRAYER) {
            _display.setFont(&FreeSansBold9pt7b);
        }
    }
    */
/*
    // For normal mode only: render progress circle and countdown
    if (mode != DisplayMode::CURRENT_PRAYER) {
        // Calculate progress for the circle
        time_t nextPrayerTime = prayers[nextPrayerIndex].time;
        time_t prevPrayerTime = prayers[currentPrayerIndex].time;
        time_t totalInterval = nextPrayerTime - prevPrayerTime;
        time_t elapsed = now - prevPrayerTime;
        float progress = 1.0f - (float)elapsed / totalInterval;

        // Draw the progress circle
        int16_t circleRadius = 150;
        int16_t circleCenterX = _display.width() / 2;
        int16_t circleCenterY = _display.height() / 4 + 50;
        renderProgressCircle(circleCenterX, circleCenterY, circleRadius, progress);

        // Render countdown text
        _display.setFont(&FreeSansBold18pt7b);
        _display.setTextColor(GxEPD_BLACK);
        _renderer.drawString(circleCenterX, circleCenterY - 20, 
                           formatCountdown(prayers[nextPrayerIndex].time), CENTER);
        _display.setFont(&FreeSans9pt7b);
        _renderer.drawString(circleCenterX, circleCenterY + 10, "left for", CENTER);
        _display.setFont(&FreeSansBold18pt7b);
        _renderer.drawString(circleCenterX, circleCenterY + 50, 
                           prayers[currentPrayerIndex].name, CENTER);
    } else {
        // Current prayer mode: show large text with current prayer info
        _display.setFont(&FreeSansBold24pt7b);
        _display.setTextColor(GxEPD_BLACK);
        _renderer.drawString(_renderL + (_renderR - _renderL)/2, 
                           _renderT + 120, prayers[currentPrayerIndex].name, CENTER);
        
        _display.setFont(&FreeSansBold18pt7b);
        String startTime = "Started at " + formatTime(prayers[currentPrayerIndex].time);
        _renderer.drawString(_renderL + (_renderR - _renderL)/2, 
                           _renderT + 170, startTime, CENTER);
    }

    // Add current time in top right (common to both modes)
    char timeStr[8];
    strftime(timeStr, sizeof(timeStr), "%I:%M%p", localtime(&now));
    String formattedTime = String(timeStr);
    if (formattedTime.startsWith("0")) {
        formattedTime = formattedTime.substring(1);
    }
    formattedTime.toLowerCase();
    
    _display.setFont(&FreeSansBold18pt7b);
    _display.setTextColor(GxEPD_BLACK);
    _renderer.drawString(_display.width() - X_MARGIN, Y_MARGIN + 20, 
                        formattedTime, RIGHT);
}
*/

/*
    // Update current time and progress calculation
    now = time(nullptr);
    updateCurrentAndNextPrayer();
        
    //Next prayer countdown including circular progress bar
    // Calculate progress for the circle
    time_t now = time(nullptr);
    time_t nextPrayerTime = prayers[nextPrayerIndex].time;
    time_t prevPrayerTime = prayers[currentPrayerIndex].time;
    time_t totalInterval = nextPrayerTime - prevPrayerTime;
    time_t elapsed = now - prevPrayerTime;
    float progress = 1.0f - (float)elapsed / totalInterval;  // Inverted progress (countdown)
    Serial.printf("Progress: %.2f, Elapsed: %ld, Total: %ld\n", progress, elapsed, totalInterval);

    // Draw the progress circle
    int16_t circleRadius = 150;  // Adjust size as needed
    int16_t circleCenterX = _display.width() / 2;
    int16_t circleCenterY = _display.height() / 4 + 50;
    

    renderProgressCircle(circleCenterX, circleCenterY, circleRadius, progress);

    // Render countdown text in the center of the circle
    _display.setFont(&FreeSansBold18pt7b);
    _display.setTextColor(GxEPD_BLACK);
    _renderer.drawString(circleCenterX, circleCenterY - 20, formatCountdown(prayers[nextPrayerIndex].time), CENTER);
    _display.setFont(&FreeSans9pt7b);
    _renderer.drawString(circleCenterX, circleCenterY + 10, "left for", CENTER);
    _display.setFont(&FreeSansBold18pt7b);
    _renderer.drawString(circleCenterX, circleCenterY + 50, prayers[currentPrayerIndex].name, CENTER);

    // Add last updated time in top right
    char timeStr[8];  // Increased size to accommodate AM/PM
    strftime(timeStr, sizeof(timeStr), "%I:%M%p", localtime(&now));  // %I for 12-hour, %p for AM/PM
    
    // Convert AM/PM to lowercase and remove leading zero from hour
    String formattedTime = String(timeStr);
    if (formattedTime.startsWith("0")) {
        formattedTime = formattedTime.substring(1);  // Remove leading zero
    }
    formattedTime.toLowerCase();  // Convert am/pm to lowercase
    _display.setFont(&FreeSansBold18pt7b);
    _display.setTextColor(GxEPD_BLACK);
    _renderer.drawString(_display.width() - X_MARGIN, Y_MARGIN + 20 , formattedTime, RIGHT);


} */

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
    //_display.setTextColor(GxEPD_BLACK);
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
    /*ß
    // Draw prayer timeline with inverted colors
    renderPrayerTimeline(_renderL + 40, _renderT + 350, _renderWidth - 80, nextPrayerIndex, true); */

    // Render prayer timeline
    _display.setFont(&FreeSansBold9pt7b);
    renderPrayerTimeline(_renderL, _renderT, _renderR - _renderL, currentPrayerIndex);

    //renderPrayerTimeline(_renderL, _renderT, _renderR - _renderL, 
    //    (mode == DisplayMode::CURRENT_PRAYER) ? nextPrayerIndex : currentPrayerIndex);
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
    
    // Draw decorative night sky elements
    int moonCenterX = _renderL + (_renderR - _renderL)/2;
    int moonCenterY = _renderT + (_renderB - _renderT)/2;
    int moonRadius = 40;
    
    // Draw moon outline (inverted colors for night mode)
    _display.fillCircle(moonCenterX, moonCenterY, moonRadius, GxEPD_WHITE);
    _display.fillCircle(moonCenterX + 10, moonCenterY - 5, moonRadius - 5, GxEPD_BLACK); // Create crescent effect
    
    // Draw stars (white plus shapes)
    const int numStars = 20;
    for(int i = 0; i < numStars; i++) {
        int starX = _renderL + 50 + (i * ((_renderR - _renderL) - 100) / numStars);
        int starY = _renderT + 5 + ((i % 3) * 50);
        
        // Draw plus shape for stars
        _display.drawFastHLine(starX - 3, starY, 7, GxEPD_WHITE);
        _display.drawFastVLine(starX, starY - 3, 7, GxEPD_WHITE);
    }

    // Draw cloud-like formations at sides (white)
    int cloudY = moonCenterY + moonRadius + 20;
    for(int i = 0; i < 3; i++) {
        // Left cloud formation
        _display.fillRect(_renderL + (i * 30), cloudY + (i * 15), 80, 5, GxEPD_WHITE);
        // Right cloud formation
        _display.fillRect(_renderR - 80 - (i * 30), cloudY + (i * 15), 80, 5, GxEPD_WHITE);
    }

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

/* DELETE  
void PrayerTimes::renderCurrentPrayerMode() {
    if (currentPrayerIndex < 0) return;

    // Draw dome outline
    drawDomeOutline(_renderL, _renderT, _renderWidth, _renderHeight);

    // Draw date and time at top
    char dateStr[32];
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    strftime(dateStr, sizeof(dateStr), "%d %b %y", timeinfo);
    
    _display.setFont(&FreeSans9pt7b);
    _renderer.drawString(_renderL + 10, _renderT + 20, dateStr, LEFT);
    
    strftime(dateStr, sizeof(dateStr), "%I:%M %p", timeinfo);
    _renderer.drawString(_renderR - 10, _renderT + 20, dateStr, RIGHT);

    // Draw prayer name in dome
    _display.setFont(&FreeSansBold24pt7b);
    _renderer.drawString(_renderL + _renderWidth/2, _renderT + 120, 
                        prayers[currentPrayerIndex].name, CENTER);

    // Draw "Pray now"
    _display.setFont(&FreeSansBold18pt7b);
    _renderer.drawString(_renderL + _renderWidth/2, _renderT + 170, 
                        "Pray now", CENTER);

    // Draw prayer time
    _renderer.drawString(_renderL + _renderWidth/2, _renderT + 220, 
                        formatTime(prayers[currentPrayerIndex].time), CENTER);

    // Draw prayer timeline
    renderPrayerTimeline(_renderL + 40, _renderT + 350, 
                        _renderWidth - 80, currentPrayerIndex);

    // Draw random hadith
    _display.setFont(&FreeSans9pt7b);
    const char* hadith = "Abdullah ibn Mas'ud reported: I said, \"O Messenger of "
                        "Allah, which deeds are best?\" The Messenger of Allah, "
                        "peace and blessings be upon him, said, \"Prayer on time.\"";
    
    _renderer.drawMultiLnString(_renderL + _renderWidth/2, _renderT + 400, 
                               hadith, CENTER, _renderWidth - 80, 4, 20);
    
    _renderer.drawString(_renderL + _renderWidth/2, _renderT + 460, 
                        "al-Mu'jam al-Kabir 9687", CENTER);

    // Draw weather info at bottom
    _display.setFont(&FreeSans9pt7b);
    char weatherStr[32];
    snprintf(weatherStr, sizeof(weatherStr), "34 C, Sunny");
    _renderer.drawString(_renderL + 10, _renderB - 10, weatherStr, LEFT);

    // Draw battery and wifi at bottom right
    char statusStr[32];
    snprintf(statusStr, sizeof(statusStr), "%d Dec %02d:%02d", 
            timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min);
    _renderer.drawString(_renderR - 10, _renderB - 10, statusStr, RIGHT);
} */

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

//void PrayerTimes::renderPrayerTimeline(int16_t x, int16_t y, int16_t w, int currentPrayer) {

void PrayerTimes::renderPrayerTimeline(int16_t x, int16_t y, int16_t w, int currentPrayer, bool invertColors) {
    // Set heights for prayer display
    y = y + 30;
    int16_t yPrayerNames = y + 10;
    int16_t yPrayerIcons = yPrayerNames + 30;
    int16_t yPrayerTimes = yPrayerIcons + 50;
    int16_t xInterval = w / 6;
    int16_t boxheight = yPrayerTimes - y;

    // Render prayer timeline
    for (int i = 0; i < 6; i++) {
        if (i == currentPrayer) {
            _display.setTextColor(invertColors ? GxEPD_BLACK : GxEPD_WHITE);
            _display.fillRoundRect(x + i * xInterval, y - 10, 
                                 xInterval, boxheight + 20, 10, 
                                 invertColors ? GxEPD_WHITE : GxEPD_BLACK);
        } else {
            _display.setTextColor(invertColors ? GxEPD_WHITE : GxEPD_BLACK);
        }

        // Format time string
        String formattedTime = formatTime(prayers[i].time);

        // Draw stringsß
        _renderer.drawString(x + xInterval * i + xInterval/2, 
                           yPrayerNames, prayers[i].name, CENTER);
        _renderer.drawString(x + xInterval * i + xInterval/2, 
                           yPrayerTimes, formattedTime, CENTER);
    }

    // Reset text color
    _display.setTextColor(GxEPD_BLACK);


    // ==========================================================
    /*
    // Initialize base font and layout parameters
    _display.setFont(&FreeSansBold9pt7b);
    
    // Set heights for prayer display
    int16_t yPrayerNames = _renderT + 10;
    int16_t yPrayerIcons = yPrayerNames + 30;
    int16_t yPrayerTimes = yPrayerIcons + 50;
    int16_t xInterval = (_renderR - _renderL) / 6;
    int16_t boxheight = yPrayerTimes - _renderT;
    
    DisplayMode currentMode = DisplayMode::NORMAL; // This should be passed in as a parameter
    //int prayerToHighlight = isCurrentPrayerMode ? nextPrayerIndex : currentPrayerIndex;
    int prayerToHighlight = (mode == DisplayMode::CURRENT_PRAYER) ? nextPrayerIndex : currentPrayerIndex;

    // Render prayer timeline
    for (int i = 0; i < 6; i++) {
        if (i == prayerToHighlight) {
            _display.setTextColor(GxEPD_WHITE);
            _display.fillRoundRect(_renderL + i * xInterval, _renderT - 10, 
                                 xInterval, boxheight + 20, 10, GxEPD_BLACK);
        } else {
            _display.setTextColor(GxEPD_BLACK);
        }

        // Format time string
        String formattedTime = formatTime(prayers[i].time);

        // Use larger font for current prayer mode
        if (mode == DisplayMode::CURRENT_PRAYER) {
            _display.setFont(&FreeSansBold12pt7b);
        }

        _renderer.drawString(_renderL + xInterval * i + xInterval/2, 
                           yPrayerNames, prayers[i].name, CENTER);
        _renderer.drawString(_renderL + xInterval * i + xInterval/2, 
                           yPrayerTimes, formattedTime, CENTER);

        // Reset to normal font size
        if (mode == DisplayMode::CURRENT_PRAYER) {
            _display.setFont(&FreeSansBold9pt7b);
        }
    }
    */

    //===================================================================

    /*
    const int iconSize = 32;
    const int timelineHeight = 40;
    const int spacing = (w - (5 * iconSize)) / 4; // Space between icons
    
    // Draw the timeline line
    _display.drawFastHLine(x, y + iconSize/2, w, GxEPD_BLACK);
    
    // Draw each prayer point
    for(int i = 0; i < 5; i++) { // 5 daily prayers (excluding sunrise)
        int iconX = x + (i * (iconSize + spacing));
        int iconY = y;
        
        // Determine which prayer index to use (skipping sunrise)
        int prayerIndex = (i >= 1) ? i + 1 : i;
        
        // Draw background circle if this is the current prayer
        if(prayerIndex == currentPrayer) {
            _display.fillRoundRect(iconX - 4, iconY - 4, 
                                 iconSize + 8, iconSize + 35, 
                                 5, GxEPD_BLACK);
            _display.setTextColor(GxEPD_WHITE);
        } else {
            _display.setTextColor(GxEPD_BLACK);
        }
        
        // Draw prayer icon
        const unsigned char* icon = getPrayerIcon(prayers[prayerIndex].name);
        if(prayerIndex == currentPrayer) {
            _display.drawInvertedBitmap(iconX, iconY, icon, iconSize, iconSize, GxEPD_WHITE);
        } else {
            _display.drawBitmap(iconX, iconY, icon, iconSize, iconSize, GxEPD_BLACK);
        }
        
        // Draw prayer time below icon
        _display.setFont(&FreeSans9pt7b);
        String timeStr = formatTime(prayers[prayerIndex].time);
        _renderer.drawString(iconX + iconSize/2, iconY + iconSize + 20, 
                           timeStr, CENTER);
    }
    
    // Reset text color
    _display.setTextColor(GxEPD_BLACK);
    
    */
}

bool PrayerTimes::isNightTime() const {
    time_t now = time(NULL);
    // Between Isha and Fajr
    return (now >= prayers[5].time) || (now < prayers[0].time);
}

int PrayerTimes::getMinutesToNextPrayer() const {
    if (nextPrayerIndex < 0) return INT_MAX;
    
    time_t now = time(NULL);
    return (prayers[nextPrayerIndex].time - now) / 60;
}
