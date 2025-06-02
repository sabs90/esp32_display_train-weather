#ifndef SETTINGS_SERVER_H
#define SETTINGS_SERVER_H

#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include "secrets.h"

#include <Arduino.h>
#include <GxEPD2_GFX.h>
#include "renderer.h"
#include "display_modes.h"
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>

// Development mode configuration
#ifdef DEV_MODE
    // In development mode, these settings control WiFi behavior
    #define USE_HARDCODED_WIFI true
    #define SAVE_HARDCODED_TO_PREFERENCES false  // Set to false if you don't want to save to preferences
#else
    // In production mode, these settings ensure AP mode for new devices
    #define USE_HARDCODED_WIFI false
    #define SAVE_HARDCODED_TO_PREFERENCES false
#endif

// Prayer Time Calculation Methods:
// 1 = University of Islamic Sciences, Karachi
// 2 = Islamic Society of North America (ISNA)
// 3 = Muslim World League (MWL)
// 4 = Umm Al-Qura University, Makkah
// 5 = Egyptian General Authority of Survey
// 6 = Institute of Geophysics, University of Tehran
// 7 = Gulf Region
// 8 = Kuwait
// 9 = Qatar
// 10 = Majlis Ugama Islam Singapura (Singapore)
// 11 = Union Organization islamic de France
// 12 = Diyanet İşleri Başkanlığı (Turkey)
// 13 = Spiritual Administration of Muslims of Russia
// 14 = Moonsighting Committee Worldwide

// Asr School Methods:
// 0 = Shafi'i, Maliki, Ja'fari, and Hanbali (shadow length = 1)
// 1 = Hanafi (shadow length = 2)


class SettingsServer {
private:
    WebServer server;
    DNSServer dnsServer;
    Preferences preferences;
    
    struct Settings {
        float latitude = -33.8688; //Sydney, NSW
        float longitude = 151.2093; //Sydney, NSW
        //float latitude = -34.5403; //Leeton, NSW
        //float longitude = 146.4030; //Leeton, NSW
        int calculationMethod = 3; //Calc methodology per above
        int school = 1;             //0 = standard, 1 = hanafi
        bool devModeEnabled = false;
        int overrideMode = 0;
        char ssid[33] = "";        // Max SSID length is 32 chars + null terminator
        char password[64] = "";    // Max WPA2 password length is 63 chars + null terminator
    } settings;

    void handleRoot();
    void handleGetSettings();
    void handleSetSettings();
    void handleWiFiSetup();
    void loadSettings();
    void saveSettings();
    bool startAPMode();
    void handleWiFiScan();
    bool setupComplete = false;

    GxEPD2_GFX* _display;
    Renderer* _renderer;
    int16_t _renderX;
    int16_t _renderY;
    int16_t _renderWidth;
    int16_t _renderHeight;

    unsigned long _lastReconnectAttempt = 0;
    const unsigned long RECONNECT_INTERVAL = 30000; // Try every 30 seconds
    const int MAX_RECONNECT_ATTEMPTS = 3;  // Changed from 10 to 3
    const unsigned long AP_MODE_RECHECK_INTERVAL = 120000; // 120 seconds between reconnect attempts in AP mode
    unsigned long _lastAPModeReconnectAttempt = 0;
    int _reconnectAttempts = 0;
    bool _reconnectionMode = false;

public:
    SettingsServer();
    void begin();
    void handle();
    const Settings& getSettings() const { return settings; }
    bool isInAPMode = false;

    void setDisplay(GxEPD2_GFX* display, Renderer* renderer);
    void setRenderArea(int16_t x, int16_t y, int16_t w, int16_t h);
    void renderAPModeInstructions();

    bool shouldEnterAPMode();
    void renderReconnectingScreen();
    void startReconnectionTimer();
    void checkReconnection();
    bool isInReconnectionMode() const { return _reconnectionMode; }
};

#endif