#include "mode_manager.h"
#include <time.h>

// Initialize static members
DisplayMode ModeManager::overrideMode = DisplayMode::NORMAL;
DisplayMode ModeManager::previousMode = DisplayMode::NORMAL; //re:mode checker for full refresh
bool ModeManager::modeOverrideEnabled = false;

void ModeManager::setOverrideFromSettings(bool enabled, int mode) {
    modeOverrideEnabled = enabled;
    if (enabled) {
        switch (mode) {
            case 0: overrideMode = DisplayMode::NORMAL; break;
            case 1: overrideMode = DisplayMode::LOW_POWER; break;
            case 2: overrideMode = DisplayMode::ALERT; break;
            case 3: overrideMode = DisplayMode::NIGHT; break;
            case 4: overrideMode = DisplayMode::CURRENT_PRAYER; break;
            default: overrideMode = DisplayMode::NORMAL;
        }
        Serial.printf("\n=== DEVELOPER MODE OVERRIDE ===\nMode set to: %s\n===============\n", 
                     getModeString(overrideMode));
    } else {
        Serial.println("\n=== DEVELOPER MODE DISABLED ===\n");
    }
}

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
    // Fix battery testing logic later - currently it crashes the device for some reason
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

    if (prayerTimes.getCurrentPrayerIndex() != 1 && prayerTimes.getMinutesToNextPrayer() <= 15) {
    //if (prayerTimes.getMinutesToNextPrayer() <= 15) {
        Serial.println("Mode: ALERT - Approaching prayer time");
        return DisplayMode::ALERT;
    }

    Serial.println("Mode: NORMAL");
    return DisplayMode::NORMAL;
}

bool ModeManager::isApproachingPrayer(const PrayerTimes& prayerTimes) {
    int minutesToNext = prayerTimes.getMinutesToNextPrayer();
    //return minutesToNext <= 60 && minutesToNext > 15;
    return minutesToNext <= 45 && minutesToNext > 15;
}

uint64_t ModeManager::getRefreshInterval(DisplayMode mode, const PrayerTimes& prayerTimes) {
    switch (mode) {
        case DisplayMode::LOW_POWER:
            return 24 * 60 * 60; // Once per day

        case DisplayMode::NIGHT: {
            // Calculate time until next Fajr
            int minsToFajr = prayerTimes.getMinutesToNextPrayer();
            
            // Wake up a few minutes before Fajr to ensure we don't miss it
            const int WAKE_BUFFER_MINS = 20;
            
            // If we're close to Fajr, check more frequently
            if (minsToFajr <= WAKE_BUFFER_MINS) {
                return 60; // Check every minute when close to Fajr
            }
            
            // Otherwise sleep until shortly before Fajr
            // But never sleep longer than 30 minutes to handle any edge cases
            return min((uint64_t)(minsToFajr - WAKE_BUFFER_MINS) * 60, (uint64_t)(30 * 60));
        }

        case DisplayMode::ALERT:
        case DisplayMode::CURRENT_PRAYER:
            return 60; // Every minute

        case DisplayMode::NORMAL:
            // Check if we're approaching next prayer (between 30 and 15 minutes)
            if (isApproachingPrayer(prayerTimes)) {
                Serial.println("Normal mode with frequent updates - approaching prayer");
                return 60; // Update every minute when approaching prayer
            }
            
            // Check if current prayer is Sunrise (index 1)
            if (prayerTimes.getCurrentPrayerIndex() == 1) {
                int minutesToDhuhr = prayerTimes.getMinutesToNextPrayer();
                
                Serial.println("\n=== Sunrise Period Debug Info ===");
                Serial.printf("Current Prayer Index: %d\n", prayerTimes.getCurrentPrayerIndex());
                Serial.printf("Next Prayer Index: %d\n", prayerTimes.getNextPrayerIndex());
                Serial.printf("Minutes until Dhuhr: %d\n", minutesToDhuhr);
                
                if (minutesToDhuhr > 60) {
                    Serial.println("Status: More than 1 hour until Dhuhr");
                    Serial.println("Action: Setting extended interval (30 minutes)");
                    return 30 * 60; // Update every 30 minutes
                } else {
                    Serial.println("Status: Less than 1 hour until Dhuhr");
                    Serial.println("Action: Setting medium interval (10 minutes)");
                    return 10 * 60; // Update every 10 minutes when within 1 hour of dhuhr
                }
            } else {
                Serial.println("\n=== Normal Mode Debug Info ===");
                Serial.printf("Current Prayer Index: %d\n", prayerTimes.getCurrentPrayerIndex());
                Serial.printf("Next Prayer Index: %d\n", prayerTimes.getNextPrayerIndex());
                Serial.println("Action: Setting default interval (10 minutes)");
            }
        default:
            return 10 * 60; //every 10 min update
            //return 60; //for rakasib
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