// Maroubra Beach Dashboard

// base class GxEPD2_GFX can be used to pass references or pointers to the
// display instance as parameter, uses ~1.2k more code enable or disable
// GxEPD2_GFX base class.
// This must be defined before including GxEPD2_BW.h
#define ENABLE_GxEPD2_GFX 1

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_GFX.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <esp_pm.h>
#include <esp_wifi.h>
#include <lwip/apps/sntp.h>
#include <sdkconfig.h>
#include <time.h>

#include "app.h"
#include "bus.h"
#include "client_utils.h"
#include "config.h"
#include "icons.h"
#include "renderer.h"
#include "secrets.h"
#include "weather.h"
#include "weather_icons.h"
#include "prayer_times.h"
#include "time_sync.h"
#include "status_bar.h"
#include "mode_manager.h"
#include "settings_server.h"
#include "serial_commands.h"


// copy the constructor from GxEPD2display_selection.h of GxEPD_Example to here
// and adapt it to the ESP32 Driver wiring, e.g.
GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT / 2> display(
    GxEPD2_750_T7(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST,
                  PIN_EPD_BUSY));  // GDEW075T7 800x480, EK79655 (GD7965)

#if defined(DRIVER_WAVESHARE)
SPIClass hspi(HSPI);
#endif

SettingsServer settingsServer;
Renderer renderer(display);
Bus bus(display, renderer);
Weather weather(display, renderer);
PrayerTimes prayerTimes(display, renderer, settingsServer);
StatusBar statusBar(display, renderer);
// ALL 3 Apps: IApp* apps[] = {&weather, &prayerTimes, &bus};
IApp* apps[] = {&weather, &prayerTimes, &statusBar}; 

const int numApps = sizeof(apps) / sizeof(apps[0]); 

void initDisplay();
void sleep(bool forceDeepSleep = false);
void powerOffDisplay();

void handleFatalError(const uint8_t* bitmap_196x196, const String& errMsgLn1,
                      const String& errMsgLn2 = "");

bool displayInitialized = false;
uint32_t lastTimeSync = 0;
int partialRefreshCount = 0;

void setup() {
  Serial.begin(115200);
  SerialCommands::init();

  // Not sure this does anything but copied from
  // https://github.com/espressif/esp-idf/tree/master/examples/wifi/power_save
  esp_pm_config_esp32_t pm_config = {
      .max_freq_mhz = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ,
      .min_freq_mhz = 80,
      .light_sleep_enable = true};
  esp_pm_configure(&pm_config);

  // Initialize display early for potential AP mode
  initDisplay();

  // Configure settings server with display
  settingsServer.setDisplay(&display, &renderer);
  settingsServer.setRenderArea(X_MARGIN, Y_MARGIN, 
                              display.width() - 2*X_MARGIN, 
                              display.height() - 2*Y_MARGIN);

  // Initialize settings server - this will handle WiFi connection and AP mode
  settingsServer.begin();
  
  // Check if we're in AP mode or reconnection mode
  if (settingsServer.isInAPMode || settingsServer.isInReconnectionMode()) {
    Serial.println("Device is in setup mode - skipping normal initialization");
    return; // Skip the rest of setup, loop will handle AP/reconnection mode
  }

  // Only proceed with normal setup if WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected after settings server initialization");
    return;
  }

  Serial.println("WiFi connected successfully!");
  Serial.println("IP: " + WiFi.localIP().toString());

  // TIME SYNCHRONIZATION
  configTzTime(TIMEZONE, NTP_SERVER_1, NTP_SERVER_2);
  bool timeConfigured = waitForSNTPSync();
  if (!timeConfigured) {
    handleFatalError(epd_bitmap_wifi_off, "Time Synchronization Failed");
    return;
  }
  lastTimeSync = millis();

  Serial.println("setup done");
}

/* ==============================================================================================
'n' for NORMAL mode
'l' for LOW_POWER mode
'a' for ALERT mode
'd' for NIGHT mode
'p' for CURRENT_PRAYER mode
'x' to disable developer mode
'?' to see the help message
============================================================================================== */

void loop() {
    uint32_t start = millis();
    SerialCommands::handleSerialCommands();

    // Handle settings server with debugging
    static unsigned long lastDebugTime = 0;
    static int debugCounter = 0;
    
    settingsServer.handle();
    
    // Print debug info every 10 seconds when in AP/reconnection mode
    if ((settingsServer.isInAPMode || settingsServer.isInReconnectionMode()) && 
        (millis() - lastDebugTime > 10000)) {
        
        lastDebugTime = millis();
        debugCounter++;
        
        Serial.printf("\n--- DEBUG STATUS (cycle %d) ---\n", debugCounter);
        Serial.printf("Time: %lu ms\n", millis());
        Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
        Serial.printf("WiFi status: %d (%s)\n", WiFi.status(), 
                     WiFi.status() == WL_CONNECTED ? "CONNECTED" :
                     WiFi.status() == WL_NO_SSID_AVAIL ? "NO_SSID_AVAIL" :
                     WiFi.status() == WL_CONNECT_FAILED ? "CONNECT_FAILED" :
                     WiFi.status() == WL_CONNECTION_LOST ? "CONNECTION_LOST" :
                     WiFi.status() == WL_DISCONNECTED ? "DISCONNECTED" : "OTHER");
        
        if (settingsServer.isInAPMode) {
            Serial.printf("Mode: AP MODE\n");
            Serial.printf("AP SSID: %s\n", WiFi.softAPSSID().c_str());
            Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
            Serial.printf("Connected stations: %d\n", WiFi.softAPgetStationNum());
        } else if (settingsServer.isInReconnectionMode()) {
            Serial.printf("Mode: RECONNECTION MODE\n");
        }
        
        Serial.println("--- END DEBUG STATUS ---\n");
    }

    // Check if in AP mode
    if (settingsServer.isInAPMode || settingsServer.isInReconnectionMode()) {
        // In AP mode, we just need to handle the server
        // and provide a brief delay to avoid CPU overload
        delay(100);
        return;  // Skip the rest of the loop
    }

    if (!settingsServer.isInAPMode) {    

      // Apply settings to mode manager
      const auto& settings = settingsServer.getSettings();
      ModeManager::setOverrideFromSettings(settings.devModeEnabled, settings.overrideMode);
      
      //============================================================================================
      // For development: force a specific mode
      // Modes: NORMAL - OK, LOW_POWER - fix, NIGHT - OK, CURRENT_PRAYER - OK, ALERT - OK
      //ModeManager::setOverrideMode(DisplayMode::NORMAL);
      //============================================================================================

      // WiFi and time sync checks
      if (WiFi.status() != WL_CONNECTED) {
          Serial.println("Reconnecting to WiFi");
          WiFi.reconnect();
      }

      if (millis() - lastTimeSync > 60 * 60 * 1000) {
          Serial.println("Re-synchronizing time");
          waitForSNTPSync();
          lastTimeSync = millis();
      }

      // Fetch data for all apps
      for (int i = 0; i < numApps; i++) {
        apps[i]->fetchData();
      } 
      uint32_t fetchComplete = millis();
      Serial.printf("Fetched data in %lu millis.\n", fetchComplete - start);

      // Update status bar values
      uint32_t batVoltage = readBatteryVoltage();
      uint32_t batPercent = calcBatPercent(batVoltage, CRIT_LOW_BATTERY_VOLTAGE, MAX_BATTERY_VOLTAGE);
      statusBar.updateValues(time(NULL), WiFi.RSSI(), batPercent);

      // Determine current mode
      DisplayMode currentMode = ModeManager::determineMode(batPercent, prayerTimes);

      
      /*
      // Use the switch just to determine the render areas and change the apps if needed. 
      switch (currentMode) {
          case DisplayMode::LOW_POWER:
              //prayerTimes.fetchData();
              prayerTimes.setRenderArea(X_MARGIN, Y_MARGIN, 
                  display.width() - X_MARGIN, display.height() - Y_MARGIN);
              prayerTimes.renderModeSpecific(currentMode);
              break;

          case DisplayMode::ALERT:
          case DisplayMode::NIGHT:
          case DisplayMode::CURRENT_PRAYER:
              //prayerTimes.fetchData();
              prayerTimes.setRenderArea(X_MARGIN, Y_MARGIN, 
                  display.width() - X_MARGIN, display.height() - Y_MARGIN);
              prayerTimes.renderModeSpecific(currentMode);
              break;

          case DisplayMode::NORMAL:
          default:
              // Normal mode - fetch and render all apps
              for (int i = 0; i < numApps; i++) {
                  apps[i]->fetchData();
              }
      } */

      // Render apps
      
    // Render
    initDisplay();
    do {
      display.fillScreen(GxEPD_WHITE);
      // margin for ikea frame    
      
      // Set render areas for each app. Left, Top, Right, Bottom. 
      int16_t oneThirdHeight = (display.height() - Y_MARGIN *2) / 3;
      int16_t marginSpacing = 20;
      int16_t Statusbarheight = 15;

      //weather.setRenderArea(X_MARGIN, Y_MARGIN, display.width() - X_MARGIN, oneThirdHeight);
      prayerTimes.setRenderArea(X_MARGIN, display.height()/2 + marginSpacing, display.width() - X_MARGIN, display.height()/2 - Y_MARGIN);
      bus.setRenderArea(X_MARGIN, Y_MARGIN + 2 * oneThirdHeight - 20, display.width() - X_MARGIN, display.height() - Y_MARGIN - 30);
      statusBar.setRenderArea(X_MARGIN, display.height() - Y_MARGIN - 20, display.width() - X_MARGIN, display.height() - Y_MARGIN);
      weather.setRenderArea(X_MARGIN, Y_MARGIN + 5, display.width() /2 , 50);

      // Update current time for prayer times
      // Render all apps
      for (int i = 0; i < numApps; i++) {
        apps[i]->render();
      }
      Serial.println("Display render complete");
    } while (display.nextPage());

      uint32_t renderComplete = millis();
      Serial.printf("Total time taken: %lu millis.\n", renderComplete - start);

      uint64_t refreshInterval = ModeManager::getRefreshInterval(currentMode, prayerTimes);
      sleep(refreshInterval > DEEP_SLEEP_THRESHOLD);
    }
}

/* Initialize e-paper display */
void initDisplay() {
  if (!displayInitialized) {
#ifdef DRIVER_WAVESHARE
    display.init(115200, true, 2, false);
    // remap spi for waveshare
    SPI.end();
    SPI.begin(PIN_EPD_SCK, PIN_EPD_MISO, PIN_EPD_MOSI, PIN_EPD_CS);
#endif
#ifdef DRIVER_DESPI_C02
    pinMode(PIN_EPD_PWR, OUTPUT);
    digitalWrite(PIN_EPD_PWR, HIGH);
    display.init(115200, true, 10, false);
#endif
    displayInitialized = true;
  }

  display.setRotation(3);
  display.setTextSize(1);
  display.setTextColor(GxEPD_BLACK);
  display.setTextWrap(false);

  // Updated refresh method
  DisplayMode currentMode = ModeManager::determineMode(0, prayerTimes);
  bool modeChanged = ModeManager::hasModeChanged(currentMode);
  
  if (modeChanged || partialRefreshCount == 0 || partialRefreshCount > 10) {
      Serial.println("Full refresh - " + String(modeChanged ? "Mode changed" : "Regular interval"));
      display.setFullWindow();
      partialRefreshCount = 1;
  } else {
      Serial.println("Partial refresh " + String(partialRefreshCount));
      display.setPartialWindow(0, 0, display.width(), display.height());
      partialRefreshCount++;
  }

  /* Previous partial refresh method 
  if (partialRefreshCount == 0 || partialRefreshCount > 10) {
    Serial.println("Full refresh");
    display.setFullWindow();
    partialRefreshCount = 1;
  } else {
    Serial.println("Partial refresh " + String(partialRefreshCount));
    display.setPartialWindow(0, 0, display.width(), display.height());
    partialRefreshCount++;
  }
  */
  display.firstPage();
  return;
}  // end initDisplay

void handleFatalError(const uint8_t* bitmap_196x196, const String& errMsgLn1,
                      const String& errMsgLn2) {
  Serial.println(errMsgLn1);
  initDisplay();
  do {
    renderer.drawError(bitmap_196x196, errMsgLn1, errMsgLn2);
  } while (display.nextPage());
  sleep(true);
}

void sleep(bool forceDeepSleep) {
    // Get refresh interval solely from the mode manager
    DisplayMode currentMode = ModeManager::determineMode(0, prayerTimes);
    uint64_t sleepDuration = ModeManager::getRefreshInterval(currentMode, prayerTimes);
    
    // Debug logging
    Serial.println("\n=== Sleep Duration Calculation ===");
    Serial.printf("Sleep duration: %lu seconds\n", sleepDuration);
    Serial.printf("Current mode: %s\n", ModeManager::getModeString(currentMode));
    Serial.println("================================\n");

    // Add delay to ensure display update completes
    delay(1000);  // 1s delay

    if (forceDeepSleep || sleepDuration > DEEP_SLEEP_THRESHOLD) {
        powerOffDisplay();
        Serial.println("Entering deep sleep for " + String(sleepDuration) + "s");
        esp_sleep_enable_timer_wakeup(sleepDuration * 1000000ULL);
        esp_deep_sleep_start();
    } else {
        Serial.println("Entering delay for " + String(sleepDuration) + "s");
        delay(sleepDuration * 1000);
    }
}

/* Old sleep function
void sleep(bool forceDeepSleep) {
  uint64_t sleepDuration = calculateSleepDuration();
  if (forceDeepSleep || sleepDuration > DEEP_SLEEP_THRESHOLD) {
    powerOffDisplay();
    Serial.println("Entering deep sleep for " + String(sleepDuration) + "s");
    esp_sleep_enable_timer_wakeup(sleepDuration * 1000000ULL);
    esp_deep_sleep_start();
  } else {
    Serial.println("Entering delay for " + String(sleepDuration) + "s");
    // Light sleep will save a lot of power but seems to kill the wifi
    // connection permanently.
    // https://www.reddit.com/r/esp32/comments/rncows/light_sleep_and_wifi/
    // esp_wifi_stop();
    // esp_sleep_enable_timer_wakeup(sleepDuration * 1000000ULL);
    // esp_light_sleep_start();
    delay(sleepDuration * 1000);
  }
}
*/

/* Power-off e-paper display */
void powerOffDisplay() {
  display.hibernate();  // turns powerOff() and sets controller to deep sleep
                        // for minimum power use
  digitalWrite(PIN_EPD_PWR, LOW);
  return;
}  // end powerOffDisplay
