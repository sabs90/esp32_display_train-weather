#include "mode_manager.h"
#include <time.h>

// Initialize static members
DisplayMode ModeManager::overrideMode = DisplayMode::NORMAL;
bool ModeManager::modeOverrideEnabled = false;

void ModeManager::setOverrideMode(DisplayMode mode) {
    overrideMode = mode;
    modeOverrideEnabled = true;
    Serial.println("\n=== DEVELOPMENT MODE OVERRIDE ===");
    Serial.printf("Override mode set to: %s\n", getModeString(mode));
    Serial.println("Automatic mode determination disabled");
    Serial.println("================================\n");
}

void ModeManager::clearOverrideMode() {
    modeOverrideEnabled = false;
    Serial.println("\n=== DEVELOPMENT MODE CLEARED ===");
    Serial.println("Returning to automatic mode determination");
    Serial.println("===============================\n");
}

bool ModeManager::isInOverrideMode() {
    return modeOverrideEnabled;
}

DisplayMode ModeManager::determineMode(uint32_t batteryPercent, const PrayerTimes& prayerTimes) {
    // Check for development override
    if (modeOverrideEnabled) {
        Serial.println("\n--- Mode Determination ---");
        Serial.println("Development override active");
        Serial.printf("Current mode (forced): %s\n", getModeString(overrideMode));
        Serial.printf("Battery: %d%%\n", batteryPercent);
        Serial.printf("Minutes to next prayer: %d\n", prayerTimes.getMinutesToNextPrayer());
        Serial.println("------------------------\n");
        return overrideMode;
    }

    // Normal mode determination logic
    /* Remove battery testing logic 
    if (batteryPercent < 5) {
        Serial.println("Mode: LOW_POWER - Battery critical");
        return DisplayMode::LOW_POWER;
    }
    */

    if (prayerTimes.isNightTime()) {
        Serial.println("Mode: NIGHT - Between Isha and Fajr");
        return DisplayMode::NIGHT;
    }

    if (prayerTimes.isWithinPrayerStart(15)) {
        Serial.println("Mode: CURRENT_PRAYER - Prayer just started");
        return DisplayMode::CURRENT_PRAYER;
    }

    if (prayerTimes.getMinutesToNextPrayer() <= 15) {
        Serial.println("Mode: ALERT - Approaching prayer time");
        return DisplayMode::ALERT;
    }

    Serial.println("Mode: NORMAL");
    return DisplayMode::NORMAL;
}

bool ModeManager::isApproachingPrayer(const PrayerTimes& prayerTimes) {
    int minutesToNext = prayerTimes.getMinutesToNextPrayer();
    return minutesToNext <= 60 && minutesToNext > 15;
}

uint64_t ModeManager::getRefreshInterval(DisplayMode mode, const PrayerTimes& prayerTimes) {
    switch (mode) {
        case DisplayMode::LOW_POWER:
            return 24 * 60 * 60; // Once per day

        case DisplayMode::NIGHT:
            return 0; // No refresh

        case DisplayMode::ALERT:
        case DisplayMode::CURRENT_PRAYER:
            return 60; // Every minute

        case DisplayMode::NORMAL:
            // Check if we're approaching next prayer (between 1 hour and 15 minutes)
            if (isApproachingPrayer(prayerTimes)) {
                Serial.println("Normal mode with frequent updates - approaching prayer");
                return 60; // Update every minute when approaching prayer
            }
            return 5 * 60; // Otherwise update every 5 minutes

        default:
            return 5 * 60;
    }
}

const char* ModeManager::getModeString(DisplayMode mode) {
    switch (mode) {
        case DisplayMode::NORMAL:
            return "NORMAL";
        case DisplayMode::LOW_POWER:
            return "LOW_POWER";
        case DisplayMode::ALERT:
            return "ALERT";
        case DisplayMode::NIGHT:
            return "NIGHT";
        case DisplayMode::CURRENT_PRAYER:
            return "CURRENT_PRAYER";
        default:
            return "UNKNOWN";
    }
}