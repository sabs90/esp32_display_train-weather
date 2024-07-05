#include "config.h"

#include <Arduino.h>

const int16_t X_MARGIN = 28;
const int16_t Y_MARGIN = 64;

// PINS
// The configuration below is intended for use with the project's official
// wiring diagrams using the FireBeetle 2 ESP32-E microcontroller board.
//
// Note: LED_BUILTIN pin will be disabled to reduce power draw.  Refer to your
//       board's pinout to ensure you avoid using a pin with this shared
//       functionality.
//
// ADC pin used to measure battery voltage
#if defined(DRIVER_WAVESHARE)
const uint8_t PIN_BAT_ADC = 34;  // A2 (A0 for micro-usb firebeetle)
// Pins for E-Paper Driver Board
const uint8_t PIN_EPD_BUSY = 25;  // 5 for micro-usb firebeetle
const uint8_t PIN_EPD_CS = 15;
const uint8_t PIN_EPD_RST = 26;
const uint8_t PIN_EPD_DC = 27;
const uint8_t PIN_EPD_SCK = 13;
const uint8_t PIN_EPD_MISO =
    12;  // 19 Master-In Slave-Out not used, as no data from display
const uint8_t PIN_EPD_MOSI = 14;
const uint8_t PIN_EPD_PWR = 0;  // Irrelevant if directly connected to 3.3V
#endif

#if defined(DRIVER_DESPI_C02)
const uint8_t PIN_BAT_ADC = 34;  // A2 (A0 for micro-usb firebeetle)
// Pins for E-Paper Driver Board
const uint8_t PIN_EPD_BUSY = 14;  // 5 for micro-usb firebeetle
const uint8_t PIN_EPD_CS = 13;
const uint8_t PIN_EPD_RST = 21;
const uint8_t PIN_EPD_DC = 22;
const uint8_t PIN_EPD_SCK = 18;
const uint8_t PIN_EPD_MISO =
    19;  // 19 Master-In Slave-Out not used, as no data from display
const uint8_t PIN_EPD_MOSI = 23;
const uint8_t PIN_EPD_PWR = 26;  // Irrelevant if directly connected to 3.3V
#endif

const unsigned long WIFI_TIMEOUT = 10000;  // ms, WiFi connection timeout.

const char *TIMEZONE = "AEST-10AEDT,M10.1.0,M4.1.0/3";
// NTP_SERVER_1 is the primary time server, while NTP_SERVER_2 is a fallback.
// pool.ntp.org will find the closest available NTP server to you.
const char *NTP_SERVER_1 = "pool.ntp.org";
const char *NTP_SERVER_2 = "time.nist.gov";
// If you encounter the 'Failed To Fetch The Time' error, try increasing
// NTP_TIMEOUT or select closer/lower latency time servers.
const unsigned long NTP_TIMEOUT = 20000;  // ms

const uint16_t REFRESH_SCHEDULE[24] = {0,   0,  0,  0,  0,  0,  60, 120,
                                       120, 60, 60, 60, 60, 60, 60, 60,
                                       60,  60, 60, 60, 60, 60, 0,  0};
const uint32_t DEEP_SLEEP_THRESHOLD = 5 * 60;

// BATTERY
// To protect the battery upon LOW_BATTERY_VOLTAGE, the display will cease to
// update until battery is charged again. The ESP32 will deep-sleep (consuming
// < 11μA), waking briefly check the voltage at the corresponding interval (in
// minutes). Once the battery voltage has fallen to CRIT_LOW_BATTERY_VOLTAGE,
// the esp32 will hibernate and a manual press of the reset (RST) button to
// begin operating again.
const uint32_t MAX_BATTERY_VOLTAGE = 4200;                  // (millivolts)
const uint32_t WARN_BATTERY_VOLTAGE = 3400;                 // (millivolts)
const uint32_t LOW_BATTERY_VOLTAGE = 3200;                  // (millivolts)
const uint32_t VERY_LOW_BATTERY_VOLTAGE = 3100;             // (millivolts)
const uint32_t CRIT_LOW_BATTERY_VOLTAGE = 3000;             // (millivolts)
const unsigned long LOW_BATTERY_SLEEP_INTERVAL = 30;        // (minutes)
const unsigned long VERY_LOW_BATTERY_SLEEP_INTERVAL = 120;  // (minutes)
