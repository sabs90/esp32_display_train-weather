#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <GxEPD2_GFX.h>
#include "renderer.h"
#include "app.h"


class Weather : public IApp {
public:
    Weather(GxEPD2_GFX& display, Renderer& renderer);
    
    // IApp interface methods
    bool fetchData() override;
    void render() override;

    // Method to set rendering area
    void setRenderArea(int16_t x, int16_t y, int16_t w, int16_t h);


private:
    GxEPD2_GFX& _display;
    Renderer& _renderer;

    // Rendering area
    /*
    int16_t _renderX;
    int16_t _renderY;
    int16_t _renderWidth;
    int16_t _renderHeight;
    */
    int16_t _renderX, _renderY, _renderWidth, _renderHeight;    

    // Weather data
    String cityName;
    float temperature;
    float feelsLike;
    int humidity;
    String weatherDescription;
    String weatherIconCode;

    // Private methods
    bool fetchWeatherData();
    void drawWeatherIcon(int16_t x, int16_t y);

    // You might want to add these methods if you need more flexibility in rendering
    void renderWeather(int16_t x, int16_t y, int16_t w, int16_t h);
};


/*
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

*/

#endif // WEATHER_H