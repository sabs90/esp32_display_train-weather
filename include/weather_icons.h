#ifndef WEATHER_ICONS_H
#define WEATHER_ICONS_H

#include <Arduino.h>

// Define icon size
#define WEATHER_ICON_WIDTH 32
#define WEATHER_ICON_HEIGHT 32

// Declare extern arrays for each weather icon
extern const unsigned char epd_bitmap_sunny[];
extern const unsigned char epd_bitmap_partly_cloudy[];
extern const unsigned char epd_bitmap_clear_sky[];
extern const unsigned char epd_bitmap_few_clouds[];

extern const unsigned char epd_bitmap_cloudy[];
extern const unsigned char epd_bitmap_rain[];
extern const unsigned char epd_bitmap_thunderstorm[];
extern const unsigned char epd_bitmap_snow[];
extern const unsigned char epd_bitmap_mist[];
extern const unsigned char epd_bitmap_fog[];
extern const unsigned char epd_bitmap_light_rain[];
extern const unsigned char epd_bitmap_heavy_rain[];
extern const unsigned char epd_bitmap_sleet[];
extern const unsigned char epd_bitmap_light_snow[];
extern const unsigned char epd_bitmap_heavy_snow[];
extern const unsigned char epd_bitmap_tornado[];
extern const unsigned char epd_bitmap_windy[];

// Function to get the appropriate bitmap based on the OpenWeather icon code
const unsigned char* getWeatherBitmap(const String& iconCode);

// Optional: You can add more utility functions here if needed
// For example:
// bool isNightTimeIcon(const String& iconCode);
// String getWeatherDescription(const String& iconCode);

#endif // WEATHER_ICONS_H