#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include "display_modes.h"
#include "prayer_times.h"

class ModeManager {
public:
    static DisplayMode determineMode(uint32_t batteryPercent, const PrayerTimes& prayerTimes);
    static uint64_t getRefreshInterval(DisplayMode mode, const PrayerTimes& prayerTimes);
    static const char* getModeString(DisplayMode mode);

    // Development mode override functions
    static void setOverrideMode(DisplayMode mode);
    static void clearOverrideMode();
    static bool isInOverrideMode();
    static void setOverrideFromSettings(bool enabled, int mode);

    static bool hasModeChanged(DisplayMode currentMode) {
        bool changed = currentMode != previousMode;
        previousMode = currentMode;
        return changed;
    }

private:
    static bool isApproachingPrayer(const PrayerTimes& prayerTimes);
    static DisplayMode overrideMode;
    static bool modeOverrideEnabled;
    //Check current mode to update between mode changes
    static DisplayMode previousMode;

};

#endif
