#ifndef STATUS_BAR_H
#define STATUS_BAR_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <GxEPD2_GFX.h>
#include "renderer.h"
#include "app.h"
#include <time.h>

class StatusBar : public IApp {
public:
    StatusBar(GxEPD2_GFX& display, Renderer& renderer);
    
    // Implement IApp interface
    bool fetchData() override;
    void setRenderArea(int16_t l, int16_t t, int16_t r, int16_t b);
    void render() override;

    // Update status bar values
    void updateValues(time_t lastUpdatedTime, int rssi, uint32_t batPercent);

private:
    GxEPD2_GFX& _display;
    Renderer& _renderer;
    int16_t _renderL;
    int16_t _renderT;
    int16_t _renderR;
    int16_t _renderB;
    
    time_t _lastUpdatedTime;
    int _rssi;
    uint32_t _batPercent;
};

#endif // STATUS_BAR_H