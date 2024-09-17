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

Weather::Weather(GxEPD2_GFX& display, Renderer& renderer)
    : _display(display), _renderer(renderer) {}

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
        DynamicJsonDocument doc(1024);
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

void Weather::render(int16_t x, int16_t y, int16_t w, int16_t h) {
    _display.setFont(&FreeSans9pt7b);
    _display.setTextColor(GxEPD_BLACK);

    // Draw city name
    _display.setCursor(x, y + 20);
    _display.print(cityName);

    // Draw temperature
    _display.setCursor(x, y + 50);
    _display.print(String(temperature, 1));
    _display.print("°C");

    // Draw weather description
    _display.setCursor(x, y + 80);
    _display.print(weatherDescription);

    // Draw humidity
    _display.setCursor(x, y + 110);
    _display.print("Humidity: ");
    _display.print(humidity);
    _display.print("%");

    // TODO: Add weather icon rendering based on weatherIconCode
}