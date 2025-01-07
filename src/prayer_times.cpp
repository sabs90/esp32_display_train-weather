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

void PrayerTimes::setRenderArea(int16_t l, int16_t t, int16_t r, int16_t b) {
    _renderL = l;
    _renderT = t;
    _renderR = r;
    _renderB = b;
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

        y += lineHeight;
    }

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
