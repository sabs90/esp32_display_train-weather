#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <GxEPD2_GFX.h>
#include "renderer.h"
#include "app.h"


class Weather {
public:
    Weather(GxEPD2_GFX& display, Renderer& renderer);
    bool fetchData();
    void render(int16_t x, int16_t y, int16_t w, int16_t h);

private:
    GxEPD2_GFX& _display;
    Renderer& _renderer;

    String cityName;
    float temperature;
    float feelsLike;
    int humidity;
    String weatherDescription;
    String weatherIconCode;

    bool fetchWeatherData();
};

#endif // WEATHER_H