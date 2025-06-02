#include "weather.h"
#include <WiFiClientSecure.h>

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
#include <HTTPClient.h>
#include <StreamUtils.h>
#include <time.h>

#include "bus_icons.h"
#include "client_utils.h"
#include "renderer.h"
#include "secrets.h"
#include "weather_icons.h"

/*
Weather::Weather(GxEPD2_GFX& display, Renderer& renderer)
    : _display(display), _renderer(renderer) {}
*/

Weather::Weather(GxEPD2_GFX& display, Renderer& renderer)
    : _display(display), _renderer(renderer), _renderX(0), _renderY(0), _renderWidth(0), _renderHeight(0) {}


bool Weather::fetchData() {
    return fetchWeatherData();
}

bool Weather::fetchWeatherData() {
    WiFiClientSecure client;
    HTTPClient http;

    client.setInsecure(); // Ignore SSL certificate validation

    String url = "https://api.openweathermap.org/data/2.5/weather?q=Punchbowl,au&units=metric&appid=" + String(OPENWEATHER_API_KEY);
    // LEETON String url = "https://api.openweathermap.org/data/2.5/weather?q=Leeton,au&units=metric&appid=" + String(OPENWEATHER_API_KEY);


    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode > 0) {
        String payload = http.getString();
        //DynamicJsonDocument doc(1024);
        JsonDocument doc;
        deserializeJson(doc, payload);

        // cityName = doc["name"].as<String>();
        temperature = doc["main"]["temp"].as<float>();
        feelsLike = doc["main"]["feels_like"].as<float>();
        humidity = doc["main"]["humidity"].as<int>();
        weatherDescription = doc["weather"][0]["description"].as<String>();
        weatherIconCode = doc["weather"][0]["icon"].as<String>();
        Serial.printf("Error on HTTP request: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return true;
    } else {
        Serial.printf("Error on HTTP request: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }
}

void Weather::setRenderArea(int16_t x, int16_t y, int16_t w, int16_t h) {
    _renderX = x;
    _renderY = y+ 13;
    _renderWidth = w;
    _renderHeight = h;
}

const WeatherIconMapping weatherIconMappings[] = {
    {"01d", "clear sky", epd_bitmap_clear_sky},
    {"01n", "clear sky", epd_bitmap_clear_sky},
    {"02d", "few clouds", epd_bitmap_few_clouds},
    {"02n", "few clouds", epd_bitmap_few_clouds},
    {"03d", "scattered clouds", epd_bitmap_cloudy},
    {"03n", "scattered clouds", epd_bitmap_cloudy},
    {"04d", "broken clouds", epd_bitmap_cloudy},
    {"04n", "broken clouds", epd_bitmap_cloudy},
    {"09d", "shower rain", epd_bitmap_rain},
    {"09n", "shower rain", epd_bitmap_rain},
    {"10d", "rain", epd_bitmap_rain},
    {"10n", "rain", epd_bitmap_rain},
    {"11d", "thunderstorm", epd_bitmap_thunderstorm},
    {"11n", "thunderstorm", epd_bitmap_thunderstorm},
    {"13d", "snow", epd_bitmap_snow},
    {"13n", "snow", epd_bitmap_snow},
    {"50d", "mist", epd_bitmap_mist},
    {"50n", "mist", epd_bitmap_mist},
    {nullptr, nullptr, nullptr}  // Sentinel to mark the end of the array
};

/*
void Weather::drawWeatherIcon(int16_t x, int16_t y) {
    // This is a basic implementation. You'll need to modify this based on your specific weather icons and how you want to display them.
    const unsigned char* icon = nullptr;
    
    // Select the appropriate icon based on the weatherIconCode
    if (weatherIconCode == "01d" || weatherIconCode == "01n") {
        icon = epd_bitmap_clear_sky; // You need to define this in weather_icons.h
    } else if (weatherIconCode == "02d" || weatherIconCode == "02n") {
        icon = epd_bitmap_few_clouds; // You need to define this in weather_icons.h
    }
    // Add more conditions for other weather codes

    // If an icon was selected, draw it
    if (icon != nullptr) {
        _display.drawBitmap(x, y, icon, WEATHER_ICON_WIDTH, WEATHER_ICON_HEIGHT, GxEPD_BLACK);
    } else {
        // If no icon matches, you could draw a default icon or just leave it blank
        _display.fillRect(x, y, WEATHER_ICON_WIDTH, WEATHER_ICON_HEIGHT, GxEPD_WHITE);
    }
}
*/

void Weather::drawWeatherIcon(int16_t x, int16_t y) {
    const unsigned char* icon = nullptr;
    
    // Find the matching icon in the mappings
    for (int i = 0; weatherIconMappings[i].code != nullptr; i++) {
        if (weatherIconCode == weatherIconMappings[i].code) {
            icon = weatherIconMappings[i].icon;
            break;
        }
    }

    // If an icon was selected, draw it
    if (icon != nullptr) {
        _display.drawBitmap(x, y, icon, WEATHER_ICON_WIDTH, WEATHER_ICON_HEIGHT, GxEPD_BLACK);
    } else {
        // If no icon matches, draw a default icon or leave it blank
        _display.fillRect(x, y, WEATHER_ICON_WIDTH, WEATHER_ICON_HEIGHT, GxEPD_WHITE);
    }
}


void Weather::render() {
    _display.setFont(&FreeSansBold18pt7b);

    // Create combined string with weather description and temperature
    //String weatherInfo = weatherDescription + ", " + String(temperature, 1) + "°C";
    String weatherInfo = String(temperature, 1) + "°C";

    // Convert first letter of weather description to uppercase
    if (weatherInfo.length() > 0) {
        weatherInfo.setCharAt(0, toupper(weatherInfo.charAt(0)));
    }

    _display.setCursor(_renderX, _renderY);
    _display.print(weatherInfo);

    /*
    // Draw humidity separately if still needed
    _display.setCursor(_renderX + 60, _renderY);
    _display.print(humidity);
    _display.print("%");
    */
}
