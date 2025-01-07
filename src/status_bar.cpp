#include "status_bar.h"
#include "config.h"
#include "display_utils.h"
#include <Fonts/FreeSans9pt7b.h>


#include "status_bar.h"
#include "config.h"
#include "display_utils.h"

StatusBar::StatusBar(GxEPD2_GFX& display, Renderer& renderer)
    : _display(display), _renderer(renderer), _lastUpdatedTime(0), _rssi(0), _batPercent(0) {}

bool StatusBar::fetchData() {
    return true;
}

void StatusBar::setRenderArea(int16_t l, int16_t t, int16_t r, int16_t b) {
    _renderL = l;
    _renderT = t;
    _renderR = r;
    _renderB = b;
    Serial.printf("Status Bar render area set: l=%d, t=%d, r=%d, b=%d (height=%d)\n", 
                 l, t, r, b, b-t);
}

void StatusBar::updateValues(time_t lastUpdatedTime, int rssi, uint32_t batPercent) {
    _lastUpdatedTime = lastUpdatedTime;
    _rssi = rssi;
    _batPercent = batPercent;
}

void StatusBar::render() {
    
    // Calculate baseline for text - assume 18px offset from bottom
    int16_t baseline = _renderB - 2; 
    
    Serial.printf("Status Bar rendering: baseline=%d, height=%d\n", 
                 baseline, _renderB - _renderT);
                 
    // Draw the actual status bar
    _display.setFont(&FreeSans9pt7b);
    _renderer.drawStatusBar(baseline, _lastUpdatedTime, _rssi, _batPercent);
}