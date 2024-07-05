
#include "display_utils.h"

#include <Arduino.h>

#include "config.h"
/*
 * Returns the wifi signal strength descriptor text for the given RSSI.
 */
const char *getWiFiDesc(int rssi) {
  if (rssi == 0) {
    return "No Connection";
  } else if (rssi >= -50) {
    return "Excellent";
  } else if (rssi >= -60) {
    return "Good";
  } else if (rssi >= -70) {
    return "Fair";
  } else {  // rssi < -70
    return "Weak";
  }
}  // end getWiFiDesc

const float getWifiFrac(int rssi) {
  if (rssi == 0) {
    return 0.0f;
  } else if (rssi >= -50) {
    return 1.0f;
  } else if (rssi >= -60) {
    return 0.8f;
  } else if (rssi >= -70) {
    return 0.6f;
  } else {  // rssi < -70
    return 0.4f;
  }
}
