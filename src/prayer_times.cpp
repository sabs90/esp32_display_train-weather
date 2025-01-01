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


/*
bool PrayerTimes::fetchPrayerTimes() {
    WiFiClientSecure client;
    HTTPClient http;

    client.setInsecure(); // Ignore SSL certificate validation

    // Replace with your location's latitude and longitude
    String url = "http://api.aladhan.com/v1/timings?latitude=-33.8688&longitude=151.2093&method=3";

    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode > 0) {
        Serial.printf("HTTP GET request successful, code: %d\n", httpCode);
        String payload = http.getString();
        Serial.println("Response payload: " + payload);
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
            http.end();
            return false;
        }

        // Debug print
        Serial.println("API Response:");
        serializeJsonPretty(doc, Serial);

        const char* prayerNames[] = {"Fajr", "Sunrise", "Dhuhr", "Asr", "Maghrib", "Isha"};
        for (int i = 0; i < 6; i++) {
            String timeStr = doc["data"]["timings"][prayerNames[i]].as<String>();
            prayers[i].name = prayerNames[i];
            prayers[i].time = parseTime(timeStr);
            
            // Debug print
            Serial.printf("%s time: %s\n", prayerNames[i], timeStr.c_str());
        }

        /*
        for (int i = 0; i < 6; i++) {
            prayers[i].time = parseTime(doc["data"]["timings"][prayers[i].name].as<String>());
        }
        */
        /*
        now = time(nullptr);
        updateCurrentAndNextPrayer();

        http.end();
        return true;
    } else {
        Serial.printf("Error on HTTP request: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }
}
*/
/* Old way of rendering
void PrayerTimes::setRenderArea(int16_t x, int16_t y, int16_t w, int16_t h) {
    _renderL = x;
    _renderT = y;
    _renderR = w;
    _renderB = h;
}

void PrayerTimes::render() {
    _display.setFont(&FreeSansBold12pt7b);
    
    int16_t y = _renderT + 30;
    int16_t lineHeight = 35;

    _display.setCursor(_renderL, y);
    _display.print("Fajr: " + fajr);
    
    y += lineHeight;
    _display.setCursor(_renderL, y);
    _display.print("Dhuhr: " + dhuhr);
    
    y += lineHeight;
    _display.setCursor(_renderL, y);
    _display.print("Asr: " + asr);
    
    y += lineHeight;
    _display.setCursor(_renderL, y);
    _display.print("Maghrib: " + maghrib);
    
    y += lineHeight;
    _display.setCursor(_renderL, y);
    _display.print("Isha: " + isha);
}
*/

void PrayerTimes::setRenderArea(int16_t l, int16_t t, int16_t r, int16_t b) {
    _renderL = l;
    _renderT = t;
    _renderR = r;
    _renderB = b;
}

void PrayerTimes::render() {
    // Update current time
    now = time(nullptr);
    updateCurrentAndNextPrayer();

    _display.setFont(&FreeSans9pt7b);
    
    // set your heights
    int16_t yPrayerNames = _renderT + 10;
    int16_t yPrayerIcons = yPrayerNames + 50;
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
            //_display.fillRoundRect(l, y, r - l, 36, 4, GxEPD_BLACK);
            _display.fillRoundRect(_renderL + i * xInterval, _renderT, xInterval, boxheight, 10, GxEPD_BLACK);
            //_display.fillRect(_renderL - 5, y - 20, _renderR - 10, lineHeight, GxEPD_BLACK);
            //_renderL + (i + 1) * xInterval, 
        } else {
            _display.setTextColor(GxEPD_BLACK);
        }

        char timeStr[6];
        strftime(timeStr, sizeof(timeStr), "%H:%M", localtime(&prayers[i].time));

        /*
        _display.setCursor(_renderL, y);
        _display.print(prayers[i].name + ": " + String(timeStr));
        */

         _renderer.drawString(_renderL + xInterval * i + xInterval/2, yPrayerNames, prayers[i].name, CENTER);
         _renderer.drawString(_renderL + xInterval * i + xInterval/2, yPrayerTimes, String(timeStr), CENTER);
         _display.drawInvertedBitmap(_renderL + xInterval * i + xInterval/2 , yPrayerIcons,  epd_bitmap_busiconplaceholder , 32, 32, GxEPD_WHITE);

         //render prayer time icon!
         
        y += lineHeight;
    }

    // Display countdown to next prayer
    _display.setTextColor(GxEPD_BLACK);
    _display.setCursor(_renderL, yPrayerTimes + 25);
    _display.print("Next: " + prayers[nextPrayerIndex].name + " in " + formatCountdown(prayers[nextPrayerIndex].time));
}


void PrayerTimes::updateCurrentAndNextPrayer() {
    now = time(nullptr);
    
    // Debug print current time
    char currentTimeStr[30];
    strftime(currentTimeStr, sizeof(currentTimeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    Serial.printf("Current time when updating prayers: %s\n", currentTimeStr);

    currentPrayerIndex = -1;
    nextPrayerIndex = 0;

    // Debug print all prayer times
    for (int i = 0; i < 6; i++) {
        char prayerTimeStr[30];
        strftime(prayerTimeStr, sizeof(prayerTimeStr), "%Y-%m-%d %H:%M:%S", localtime(&prayers[i].time));
        Serial.printf("Prayer %s time: %s\n", prayers[i].name, prayerTimeStr);
    }

    for (int i = 0; i < 6; i++) {
        if (now < prayers[i].time) {
            nextPrayerIndex = i;
            break;
        }
        currentPrayerIndex = i;
    }

    if (currentPrayerIndex == 5) {  // If Isha is current, next is tomorrow's Fajr
        nextPrayerIndex = 0;
        // Adjust next prayer time to tomorrow if it's for Fajr
        time_t fajrTime = prayers[0].time;
        struct tm fajrTm;
        localtime_r(&fajrTime, &fajrTm);
        fajrTm.tm_mday++;  // Add one day
        prayers[0].time = mktime(&fajrTm);
    }

    Serial.printf("Current prayer index: %d, Next prayer index: %d\n", 
                 currentPrayerIndex, nextPrayerIndex);

    // Debug the countdown
    if (nextPrayerIndex >= 0 && nextPrayerIndex < 6) {
        time_t diff = prayers[nextPrayerIndex].time - now;
        int hours = diff / 3600;
        int minutes = (diff % 3600) / 60;
        Serial.printf("Time until next prayer: %02d:%02d\n", hours, minutes);
    }
}

/*


void PrayerTimes::updateCurrentAndNextPrayer() {
    currentPrayerIndex = -1;
    nextPrayerIndex = 0;

    for (int i = 0; i < 6; i++) {
        if (now < prayers[i].time) {
            nextPrayerIndex = i;
            break;
        }
        currentPrayerIndex = i;
    }

    if (currentPrayerIndex == 5) {  // If Isha is current, next is tomorrow's Fajr
        nextPrayerIndex = 0;
    }
}
*/

String PrayerTimes::formatCountdown(time_t target) {
    time_t diff = target - now;
    int hours = diff / 3600;
    int minutes = (diff % 3600) / 60;
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", hours, minutes);
    return String(buffer);
}
/*
time_t PrayerTimes::parseTime(const String& timeStr) {
    struct tm tm;
    time_t now = time(nullptr);
    localtime_r(&now, &tm);
    sscanf(timeStr.c_str(), "%d:%d", &tm.tm_hour, &tm.tm_min);
    tm.tm_sec = 0;
    return mktime(&tm);
}
*/

time_t PrayerTimes::parseTime(const String& timeStr) {
    // First, debug print the input
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
    time_t now = time(nullptr);
    if (now < 24 * 60 * 60) {  // Less than Jan 1, 1970 00:00:00 + 1 day
        Serial.println("Warning: System time not set properly!");
        return 0;
    }
    
    localtime_r(&now, &timeinfo);
    
    // Store original values for comparison
    int orig_hour = timeinfo.tm_hour;
    int orig_min = timeinfo.tm_min;
    
    // Set the parsed time
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = 0;
    
    // Convert to time_t
    time_t result = mktime(&timeinfo);
    
    // If the prayer time has already passed today and it's a morning prayer
    if (result < now && hour < 12) {
        Serial.println("Prayer time has passed, adjusting to tomorrow");
        timeinfo.tm_mday++;  // Add one day
        result = mktime(&timeinfo);
    }
    
    // Debug output
    char buffer[26];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.printf("Original time - Hour: %d, Minute: %d\n", orig_hour, orig_min);
    Serial.printf("Final parsed time: %s\n", buffer);
    
    return result;
}

/*
time_t PrayerTimes::parseTime(const String& timeStr) {
    struct tm timeinfo;
    int hour, minute;
    if (sscanf(timeStr.c_str(), "%d:%d", &hour, &minute) != 2) {
        Serial.printf("Failed to parse time string: %s\n", timeStr.c_str());
        return 0;  // Return 0 for invalid time
    }
    
    time_t now = time(nullptr);
    localtime_r(&now, &timeinfo);
    
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = 0;
    
    time_t result = mktime(&timeinfo);
    
    char buffer[26];
    ctime_r(&result, buffer);
    buffer[24] = '\0';  // Remove newline character
    Serial.printf("Parsed time: %s from %s\n", buffer, timeStr.c_str());
    
    return result;
}
*/