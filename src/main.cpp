// Sydney Busses Departure Board.

// Use HSPI for EPD (and VSPI for SD) with Waveshare ESP32 Driver Board
#define USE_HSPI_FOR_EPD

// base class GxEPD2_GFX can be used to pass references or pointers to the
// display instance as parameter, uses ~1.2k more code enable or disable
// GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0

// uncomment next line to use class GFX of library GFX_Root instead of
// Adafruit_GFX #include <GFX.h> Note: if you use this with ENABLE_GxEPD2_GFX 1:
//       uncomment it in GxEPD2_GFX.h too, or add #include <GFX.h> before any
//       #include <GxEPD2_GFX.h>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <GxEPD2_BW.h>
#include <HTTPClient.h>
#include <StreamUtils.h>
#include <WiFi.h>
#include <lwip/apps/sntp.h>
#include <time.h>

#include "icons.h"
#include "secrets.h"

// copy the constructor from GxEPD2_display_selection.h of GxEPD_Example to here
// and adapt it to the ESP32 Driver wiring, e.g.
GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT / 2> display(
    GxEPD2_750_T7(/*CS=*/15, /*DC=D3*/ 27, /*RST=*/26,
                  /*BUSY=*/25));  // GDEW075T7 800x480, EK79655 (GD7965)

#if defined(ESP32) && defined(USE_HSPI_FOR_EPD)
SPIClass hspi(HSPI);
#endif

#define STOP_ID "2035143"

void connectWifi();
boolean fetchData();
void showBusStopDepartures();
std::vector<JsonObject> getSortedStopEvents();
int drawStopEvent(const JsonObject &stopEvent, int y, int xMargin = 20);
time_t getDepartureTime(const JsonObject &stopEvent);
time_t parseTimeUtc(const char *utcTimeString);

JsonDocument busStopDoc;

void setup() {
  Serial.begin(115200);
  Serial.println("setup");
  connectWifi();

  // *** special handling for Waveshare ESP32 Driver board *** //
  // ********************************************************* //
#if defined(ESP32) && defined(USE_HSPI_FOR_EPD)
  hspi.begin(13, 12, 14, 15);  // remap hspi for EPD (swap pins)
  display.epd2.selectSPI(hspi, SPISettings(4000000, MSBFIRST, SPI_MODE0));
#endif
  // *** end of special handling for Waveshare ESP32 Driver board *** //
  // **************************************************************** //

  display.init(115200);
  Serial.println("setup done");
}

void loop() {
  // Fetch
  fetchData();

  // Render
  showBusStopDepartures();

  // Loop
  delay(30 * 1000);
}

void connectWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Establishing connection to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  Serial.printf("Connected to network %s\n", WIFI_SSID);

  // Sync SNTP
  sntp_setoperatingmode(SNTP_OPMODE_POLL);

  char server[] =
      "time.nist.gov";  // sntp_setservername takes a non-const char*, so use
                        // a non-const variable to avoid warning
  sntp_setservername(0, server);
  sntp_init();

  Serial.println("Waiting for NTP time sync...");
  Serial.printf("Syncing NTP time via %s...\n", server);
  time_t now;
  while (time(&now), now < 1625099485) {
    delay(1000);
  }

  setenv("TZ", "AEST-10AEDT,M10.1.0,M4.1.0/3", 1);
  tzset();
  char buf[256];
  strftime(buf, sizeof(buf), "Got time: %Y-%m-%d %H:%M:%S\n", localtime(&now));
  Serial.printf(buf);
}

bool fetchData() {
  uint32_t start = millis();
  WiFiClientSecure client;
  HTTPClient http;

  // Construct the http request
  http.useHTTP10(true);
  http.setAuthorizationType("apiKey");
  http.setAuthorization(TFNSW_API_KEY);
  http.begin(
      "https://api.transport.nsw.gov.au/v1/tp/"
      "departure_mon?outputFormat=rapidJSON&coordOutputFormat=EPSG%3A4326&"
      "mode="
      "direct&type_dm=stop&name_dm=" STOP_ID
      "&departureMonitorMacro=true&TfNSWDM="
      "true&version=10.2.1.42",
      (const char *)NULL);

  // Send the request as a GET
  Serial.println("Sending tfnsw request");
  int http_code = http.GET();

  Serial.printf("Finished request in %lu millis.\n", millis() - start);
  if (http_code > 0) {
    Serial.printf("Response code: %d Data length: %d\n", http_code,
                  http.getSize());

    // The filter: it contains "true" for each value we want to keep
    JsonDocument filter;
    filter["locations"][0]["disassembledName"] = true;
    filter["stopEvents"][0]["departureTimePlanned"] = true;
    filter["stopEvents"][0]["departureTimeEstimated"] = true;
    filter["stopEvents"][0]["isRealtimeControlled"] = true;
    filter["stopEvents"][0]["transportation"]["disassembledName"] = true;
    filter["stopEvents"][0]["transportation"]["origin"]["name"] = true;
    filter["stopEvents"][0]["transportation"]["destination"]["name"] = true;

    // https://github.com/espressif/arduino-esp32/issues/4279
    // For some reason, getStream only returns a truncated response.
    DeserializationError err = deserializeJson(
        busStopDoc, http.getString(), DeserializationOption::Filter(filter));

    if (err) {
      http.end();
      Serial.printf("Error parsing response! %s\n", err.c_str());
      return false;
    }

    Serial.println("Parsing successful");
    http.end();
    return true;
  } else {
    Serial.printf("Error on HTTP request (%d): %s\n", http_code,
                  http.errorToString(http_code).c_str());
    http.end();
    return false;
  }
}

void showBusStopDepartures() {
  const time_t now = time(NULL);
  const int16_t xMargin = 16;
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.setRotation(1);
  display.setTextColor(GxEPD_BLACK);
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(0, 0);
    display.println();
    display.setCursor(xMargin, display.getCursorY());
    const char *stopName = busStopDoc["locations"][0]["disassembledName"];
    display.printf("%s\n", stopName);

    // TODO: choose y in a better way.
    int y = display.getCursorY();
    std::vector<JsonObject> stopEvents = getSortedStopEvents();
    for (int i = 0; i < stopEvents.size() && i < 8; i++) {
      JsonObject stopEvent = busStopDoc["stopEvents"][i];
      if (getDepartureTime(stopEvent) > now + 60 * 60) {
        break;
      }
      serializeJsonPretty(stopEvent, Serial);
      y = drawStopEvent(stopEvent, y, xMargin);
      y += 8;
      display.drawFastHLine(8 + xMargin, y,
                            display.width() - 2 * 8 - 2 * xMargin, GxEPD_BLACK);
      y += 8;
    }
    display.setFont(&FreeSans9pt7b);
    char lastUpdatedTimeString[48];
    strftime(lastUpdatedTimeString, sizeof(lastUpdatedTimeString),
             "Last Updated: %d %b %H:%M", localtime(&now));
    int16_t lux, luy;
    uint16_t luw, luh;
    display.getTextBounds(lastUpdatedTimeString, 0, 0, &lux, &luy, &luw, &luh);
    display.setCursor(display.width() - xMargin - luw, display.height() - 18);
    display.print(lastUpdatedTimeString);
  } while (display.nextPage());
}

std::vector<JsonObject> getSortedStopEvents() {
  std::vector<JsonObject> stopEvents;
  // 16 is a magic number that avoids crashes (presumably due to running out of
  // memory?)
  for (int i = 0; i < busStopDoc["stopEvents"].size() && i < 16; i++) {
    JsonObject stopEvent = busStopDoc["stopEvents"][i];
    stopEvents.push_back(stopEvent);
  }

  std::sort(stopEvents.begin(), stopEvents.end(),
            [](const JsonObject &a, const JsonObject &b) {
              return getDepartureTime(a) - getDepartureTime(b);
            });
  return stopEvents;
}

int drawStopEvent(const JsonObject &stopEvent, int y, int xMargin) {
  const time_t now = time(NULL);

  bool isRealtime =
      stopEvent["isRealtimeControlled"] && stopEvent["departureTimeEstimated"];
  const char *busName = stopEvent["transportation"]["disassembledName"];
  const char *destination = stopEvent["transportation"]["destination"]["name"];

  time_t departureTime_t = getDepartureTime(stopEvent);

  char departureTimeHM[8];
  strftime(departureTimeHM, sizeof(departureTimeHM), "%H:%M",
           localtime(&departureTime_t));

  // Find the number of minutes until the next departure
  int nextDepartureMinutes = ((int)difftime(departureTime_t, now)) / 60;
  char nextDepartureMinutesString[8];
  snprintf(nextDepartureMinutesString, sizeof(nextDepartureMinutesString), "%d",
           nextDepartureMinutes);

  display.setTextWrap(false);
  display.setFont(&FreeSansBold24pt7b);
  y += 36;

  int16_t bnbx, bnby;
  uint16_t bnbw, bnbh;
  display.getTextBounds(busName, 0, 0, &bnbx, &bnby, &bnbw, &bnbh);
  display.setCursor(xMargin, y);
  display.print(busName);

  int16_t ndmbx, ndmby;
  uint16_t ndmbw, ndmbh;
  display.getTextBounds(nextDepartureMinutesString, 0, 0, &ndmbx, &ndmby,
                        &ndmbw, &ndmbh);

  const char *minString = "  min";
  display.setFont(&FreeSansBold9pt7b);
  int16_t mbx, mby;
  uint16_t mbw, mbh;
  display.getTextBounds(minString, 0, 0, &mbx, &mby, &mbw, &mbh);

  display.setCursor(display.width() - xMargin - mbw, y);
  display.print(minString);
  display.setFont(&FreeSansBold24pt7b);
  display.setCursor(display.width() - xMargin - mbw - ndmbw, y);
  display.print(nextDepartureMinutesString);

  y += 12;

  display.setFont(&FreeSans9pt7b);
  y += 14;

  int16_t dbx, dby;
  uint16_t dbw, dbh;
  display.getTextBounds(destination, 0, 0, &dbx, &dby, &dbw, &dbh);
  display.setCursor(xMargin, y);
  display.print(destination);

  int16_t rtbx, rtby;
  uint16_t rtbw, rtbh;
  display.getTextBounds(departureTimeHM, 0, 0, &rtbx, &rtby, &rtbw, &rtbh);
  display.setCursor(display.width() - xMargin - rtbw, y);
  display.print(departureTimeHM);

  if (isRealtime) {
    display.drawInvertedBitmap(display.width() - xMargin - rtbw - 8 - 16,
                               y - 14, epd_bitmap_rssfeed, 16, 16, GxEPD_BLACK);
  }

  y += 4;

  return y;
}

time_t getDepartureTime(const JsonObject &stopEvent) {
  bool isRealtime =
      stopEvent["isRealtimeControlled"] && stopEvent["departureTimeEstimated"];

  const char *departureTimeString =
      stopEvent[isRealtime ? "departureTimeEstimated" : "departureTimePlanned"];
  return parseTimeUtc(departureTimeString);
}

time_t parseTimeUtc(const char *utcTimeString) {
  struct tm tm = {0};
  strptime(utcTimeString, "%Y-%m-%dT%H:%M:%SZ", &tm);
  return mktime(&tm) - _timezone;
}