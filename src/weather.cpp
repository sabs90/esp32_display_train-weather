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

    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode > 0) {
        String payload = http.getString();
        //DynamicJsonDocument doc(1024);
        JsonDocument doc;
        deserializeJson(doc, payload);

        cityName = doc["name"].as<String>();
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
    _renderY = y;
    _renderWidth = w;
    _renderHeight = h;
}

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

void Weather::render() { //int16_t x, int16_t y, int16_t w, int16_t h) {
    
    /*int16_t x = 0;  // or some other appropriate value
    int16_t y = 0;  // or some other appropriate value
    int16_t w = _display.width();  // or some other appropriate value
    int16_t h = _display.height() / 3;  // or some other appropriate value
    */

    //set font
    _display.setFont(&FreeSansBold18pt7b);

    // Draw weather icon
    drawWeatherIcon(_renderX, _renderY);

    // Draw city name
    _display.setCursor(_renderX + WEATHER_ICON_WIDTH + 5, _renderY + 20);
    _display.print(cityName);

    // Draw temperature
    _display.setCursor(_renderX + WEATHER_ICON_WIDTH + 5, _renderY + 50);
    _display.print(String(temperature, 1));
    _display.print("°C");

    // Draw weather description
    _display.setCursor(_renderX, _renderY + WEATHER_ICON_HEIGHT + 100);
    _display.print(weatherDescription);

    // Draw humidity
    _display.setCursor(_renderX, _renderY + WEATHER_ICON_HEIGHT + 150);
    _display.print("Humidity: ");
    _display.print(humidity);
    _display.print("%");
    

    // TODO: Add weather icon rendering based on weatherIconCode
}