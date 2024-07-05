#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <Arduino.h>

#include <cstdint>

// Use HSPI for EPD (and VSPI for SD) with Waveshare ESP32 Driver Board
// #define DRIVER_WAVESHARE
#define DRIVER_DESPI_C02

extern const int16_t X_MARGIN;
extern const int16_t Y_MARGIN;

extern const uint8_t PIN_BAT_ADC;
extern const uint8_t PIN_EPD_BUSY;
extern const uint8_t PIN_EPD_CS;
extern const uint8_t PIN_EPD_RST;
extern const uint8_t PIN_EPD_DC;
extern const uint8_t PIN_EPD_SCK;
extern const uint8_t PIN_EPD_MISO;
extern const uint8_t PIN_EPD_MOSI;
extern const uint8_t PIN_EPD_PWR;
extern const unsigned long WIFI_TIMEOUT;
extern const char *TIMEZONE;
extern const char *NTP_SERVER_1;
extern const char *NTP_SERVER_2;
extern const unsigned long NTP_TIMEOUT;
/* Number of times each hour to refresh the display */
extern const uint16_t REFRESH_SCHEDULE[24];
extern const uint32_t DEEP_SLEEP_THRESHOLD;
extern const uint32_t MAX_BATTERY_VOLTAGE;
extern const uint32_t WARN_BATTERY_VOLTAGE;
extern const uint32_t LOW_BATTERY_VOLTAGE;
extern const uint32_t VERY_LOW_BATTERY_VOLTAGE;
extern const uint32_t CRIT_LOW_BATTERY_VOLTAGE;
extern const unsigned long LOW_BATTERY_SLEEP_INTERVAL;
extern const unsigned long VERY_LOW_BATTERY_SLEEP_INTERVAL;

// CONFIG VALIDATION - DO NOT MODIFY
#if !(defined(DRIVER_WAVESHARE) ^ defined(DRIVER_DESPI_C02))
#error Invalid configuration. Exactly one driver board must be selected.
#endif

#endif