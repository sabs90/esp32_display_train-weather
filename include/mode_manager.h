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

private:
    static bool isApproachingPrayer(const PrayerTimes& prayerTimes);
    static DisplayMode overrideMode;
    static bool modeOverrideEnabled;
};

#endif

/*
#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include "display_modes.h"
#include "prayer_times.h"
class ModeManager {
public:
    static DisplayMode determineMode(uint32_t batteryPercent, const PrayerTimes& prayerTimes);
    //static uint64_t getRefreshInterval(DisplayMode mode);
    static uint64_t getRefreshInterval(DisplayMode mode, const PrayerTimes& prayerTimes);
    static const char* getModeString(DisplayMode mode);
private:
    static bool isApproachingPrayer(const PrayerTimes& prayerTimes);
};

#endif
*/