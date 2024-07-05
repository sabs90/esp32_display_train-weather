/* Client side utilities for esp32-weather-epd.
 * Copyright (C) 2022-2024  Luke Marzen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "client_utils.h"

#include <Adafruit_BusIO_Register.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_sntp.h>
#include <time.h>

#include <cstring>
#include <vector>

#include "config.h"
#include "secrets.h"

/*
 * Power-on and connect WiFi.
 *
 * Returns WiFi status.
 */
wl_status_t startWiFi() {
  WiFi.mode(WIFI_STA);
  Serial.printf("Connecting to '%s'", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // timeout if WiFi does not connect in WIFI_TIMEOUT ms from now
  unsigned long timeout = millis() + WIFI_TIMEOUT;
  wl_status_t connection_status = WiFi.status();

  while ((connection_status != WL_CONNECTED) && (millis() < timeout)) {
    Serial.print(".");
    delay(50);
    connection_status = WiFi.status();
  }
  Serial.println();

  if (connection_status == WL_CONNECTED) {
    Serial.println("IP: " + WiFi.localIP().toString());
  } else {
    Serial.printf("Could not connect to '%s'\n", WIFI_SSID);
  }
  return connection_status;
}  // startWiFi

/* Disconnect and power-off WiFi.
 */
void killWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}  // killWiFi

/* Prints the local time to serial monitor.
 *
 * Returns true if getting local time was a success, otherwise false.
 */
bool printLocalTime() {
  struct tm timeInfo = {};
  int attempts = 0;
  while (!getLocalTime(&timeInfo) && attempts++ < 3) {
    Serial.println("Failed to get time");
    return false;
  }
  char buf[256];
  strftime(buf, sizeof(buf), "Got time: %Y-%m-%d %H:%M:%S\n", &timeInfo);
  Serial.printf(buf);
  return true;
}  // printLocalTime

/* Waits for NTP server time sync, adjusted for the time zone specified in
 * config.cpp.
 *
 * Returns true if time was set successfully, otherwise false.
 *
 * Note: Must be connected to WiFi to get time from NTP server.
 */
bool waitForSNTPSync() {
  // Wait for SNTP synchronization to complete
  unsigned long timeout = millis() + NTP_TIMEOUT;
  if ((sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) &&
      (millis() < timeout)) {
    Serial.print("Waiting for SNTP sync");
    delay(100);  // ms
    while ((sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) &&
           (millis() < timeout)) {
      Serial.print(".");
      delay(100);  // ms
    }
    Serial.println();
  }
  return printLocalTime();
}  // waitForSNTPSync

/*
 * Returns battery voltage in millivolts (mv).
 */
uint32_t readBatteryVoltage() {
  esp_adc_cal_characteristics_t adc_chars;
  // __attribute__((unused)) disables compiler warnings about this variable
  // being unused (Clang, GCC) which is the case when DEBUG_LEVEL == 0.
  esp_adc_cal_value_t val_type __attribute__((unused));
  adc_power_acquire();
  uint16_t adc_val = analogRead(PIN_BAT_ADC);
  adc_power_release();

  // We will use the eFuse ADC calibration bits, to get accurate voltage
  // readings. The DFRobot FireBeetle Esp32-E V1.0's ADC is 12 bit, and uses
  // 11db attenuation, which gives it a measurable input voltage range of 150mV
  // to 2450mV.
  val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_11db,
                                      ADC_WIDTH_BIT_12, 1100, &adc_chars);

  uint32_t batteryVoltage = esp_adc_cal_raw_to_voltage(adc_val, &adc_chars);
  // DFRobot FireBeetle Esp32-E V1.0 voltage divider (1M+1M), so readings are
  // multiplied by 2.
  batteryVoltage *= 2;
  return batteryVoltage;
}  // end readBatteryVoltage

/*
 * Returns battery percentage, rounded to the nearest integer.
 * Takes a voltage in millivolts and uses a sigmoidal approximation to find an
 * approximation of the battery life percentage remaining.
 *
 * This function contains LGPLv3 code from
 * <https://github.com/rlogiacco/BatterySense>.
 *
 * Symmetric sigmoidal approximation
 * <https://www.desmos.com/calculator/7m9lu26vpy>
 *
 * c - c / (1 + k*x/v)^3
 */
uint32_t calcBatPercent(uint32_t v, uint32_t minv, uint32_t maxv) {
  // slow
  // uint32_t p = 110 - (110 / (1 + pow(1.468 * (v - minv)/(maxv - minv), 6)));

  // steep
  // uint32_t p = 102 - (102 / (1 + pow(1.621 * (v - minv)/(maxv -
  // minv), 8.1)));

  // normal
  uint32_t p = 105 - (105 / (1 + pow(1.724 * (v - minv) / (maxv - minv), 5.5)));
  return p >= 100 ? 100 : p;
}  // end calcBatPercent

uint64_t calculateSleepDuration() {
  tm timeInfo = {};
  getLocalTime(&timeInfo);

  uint64_t sleepDuration = 0;
  int curHour = timeInfo.tm_hour;
  int curSecInHour = 60 * timeInfo.tm_min + timeInfo.tm_sec;

  int refreshesThisHour = REFRESH_SCHEDULE[curHour];
  if (refreshesThisHour > 0) {
    int refreshSeconds = 3600 / refreshesThisHour;
    sleepDuration = refreshSeconds - curSecInHour % refreshSeconds;
    if (curSecInHour + sleepDuration < 3600) {
      return sleepDuration;
    }
  }
  // Get us to the end of the hour
  sleepDuration = 3600 - curSecInHour;

  // Find the next hour when we can refresh
  curHour = (curHour + 1) % 24;
  while (REFRESH_SCHEDULE[curHour] == 0) {
    sleepDuration += 3600;
    curHour = (curHour + 1) % 24;
  }
  return sleepDuration;
}
