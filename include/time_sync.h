#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <Arduino.h>
#include <time.h>

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

bool syncTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  
  Serial.println("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  int retry = 0;
  while (now < 8 * 3600 * 2 && retry < 10) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    retry++;
  }
  Serial.println();
  
  if (now < 8 * 3600 * 2) {
    Serial.println("Failed to get NTP time.");
    return false;
  }
  
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("NTP time sync completed. Current time: ");
  Serial.println(asctime(&timeinfo));
  return true;
}

#endif // TIME_SYNC_H