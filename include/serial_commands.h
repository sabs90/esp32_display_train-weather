#ifndef SERIAL_COMMANDS_H
#define SERIAL_COMMANDS_H

#include "mode_manager.h"

class SerialCommands {
public:
    static void init() {
        // Print the command menu
        Serial.println("\n=== Serial Commands Available ===");
        Serial.println("n - Switch to NORMAL mode");
        Serial.println("l - Switch to LOW_POWER mode");
        Serial.println("a - Switch to ALERT mode");
        Serial.println("d - Switch to NIGHT mode");
        Serial.println("p - Switch to CURRENT_PRAYER mode");
        Serial.println("x - Disable developer mode");
        Serial.println("? - Show this help message");
        Serial.println("==============================\n");
    }

    static void handleSerialCommands() {
        if (Serial.available()) {
            char cmd = Serial.read();
            
            // Convert to lowercase for case-insensitive commands
            cmd = tolower(cmd);

            DisplayMode newMode;
            bool validCommand = true;

            switch (cmd) {
                case 'n':
                    newMode = DisplayMode::NORMAL;
                    break;
                case 'l':
                    newMode = DisplayMode::LOW_POWER;
                    break;
                case 'a':
                    newMode = DisplayMode::ALERT;
                    break;
                case 'd':
                    newMode = DisplayMode::NIGHT;
                    break;
                case 'p':
                    newMode = DisplayMode::CURRENT_PRAYER;
                    break;
                case 'x':
                    ModeManager::clearOverrideMode();
                    Serial.println("\n=== Developer mode disabled ===");
                    Serial.println("Device will now use automatic mode selection");
                    Serial.println("===============================\n");
                    return;
                case '?':
                    init();
                    return;
                case '\n':
                case '\r':
                    // Ignore newline characters
                    return;
                default:
                    validCommand = false;
                    Serial.printf("\nInvalid command: '%c'. Type '?' for help.\n", cmd);
                    return;
            }

            if (validCommand) {
                ModeManager::setOverrideMode(newMode);
                Serial.printf("\n=== Mode changed to: %s ===\n\n", ModeManager::getModeString(newMode));
            }

            // Clear any remaining characters in the buffer
            while (Serial.available()) {
                Serial.read();
            }
        }
    }
};

#endif