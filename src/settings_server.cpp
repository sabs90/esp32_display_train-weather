#include "settings_server.h"
#include <WiFi.h> 

// AP mode settings
const char* AP_SSID = "PrayerDisplay_Setup";  // The name that will show up in WiFi networks list
const char* AP_PASSWORD = "123456789";         // The password for the AP mode
const byte DNS_PORT = 53;                     // Standard DNS port

// First, add the HTML template we discussed earlier
const char CONFIG_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>Prayer Times Settings</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; background: #f0f0f0; }
        .container { max-width: 400px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
        .field { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input, select { width: 100%; padding: 8px; margin-bottom: 10px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        input[type="checkbox"] { width: auto; }
        button { background: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; width: 100%; }
        button:hover { background: #45a049; }
        .network-settings { margin-top: 30px; padding-top: 20px; border-top: 1px solid #ddd; }
        .networks-list { max-height: 200px; overflow-y: auto; margin: 10px 0; }
        .network-item { padding: 10px; border: 1px solid #ddd; margin-bottom: 5px; cursor: pointer; border-radius: 4px; }
        .network-item:hover { background: #f5f5f5; }
        .loading { text-align: center; padding: 20px; }
        .dev-settings { margin-top: 20px; padding-top: 20px; border-top: 1px solid #ddd; }
    </style>
</head>
<body>
    <div class="container">
        <h2>Prayer Times Settings</h2>
        <form id="settingsForm">
            <div class="field">
                <label for="latitude">Latitude:</label>
                <input type="number" id="latitude" step="0.0001" required>
            </div>
            <div class="field">
                <label for="longitude">Longitude:</label>
                <input type="number" id="longitude" step="0.0001" required>
            </div>
            <div class="field">
                <label for="method">Calculation Method:</label>
                <select id="method">
                    <option value="0">Shia Ithna-Ashari</option>
                    <option value="1">University of Islamic Sciences, Karachi</option>
                    <option value="2">Islamic Society of North America</option>
                    <option value="3">Muslim World League</option>
                    <option value="4">Umm Al-Qura University, Makkah</option>
                    <option value="5">Egyptian General Authority of Survey</option>
                </select>
            </div>
            <div class="field">
                <label for="school">School:</label>
                <select id="school">
                    <option value="0">Shafi</option>
                    <option value="1">Hanafi</option>
                </select>
            </div>

            <div class="dev-settings">
                <h3>Developer Settings</h3>
                <div class="field">
                    <label for="devModeEnabled">
                        <input type="checkbox" id="devModeEnabled" style="width: auto; margin-right: 8px;">
                        Enable Developer Mode
                    </label>
                </div>
                <div class="field">
                    <label for="overrideMode">Override Mode:</label>
                    <select id="overrideMode">
                        <option value="0">Normal</option>
                        <option value="1">Low Power</option>
                        <option value="2">Alert</option>
                        <option value="3">Night</option>
                        <option value="4">Current Prayer</option>
                    </select>
                </div>
            </div>

            <div class="network-settings">
                <h3>WiFi Settings</h3>
                <div class="field">
                    <label for="ssid">Network Name:</label>
                    <input type="text" id="ssid" required>
                    <button type="button" onclick="scanNetworks()" style="margin-bottom: 10px;">Scan Networks</button>
                    <div id="networks-list" class="networks-list"></div>
                </div>
                <div class="field">
                    <label for="password">Password:</label>
                    <input type="password" id="password">
                </div>
            </div>

            <button type="submit">Save Settings</button>
        </form>
    </div>

    <script>
        // Fetch current settings when page loads
        window.onload = async () => {
            try {
                const response = await fetch('/settings');
                const settings = await response.json();
                
                document.getElementById('latitude').value = settings.latitude;
                document.getElementById('longitude').value = settings.longitude;
                document.getElementById('method').value = settings.calculationMethod;
                document.getElementById('school').value = settings.school;
                document.getElementById('ssid').value = settings.ssid || '';
                document.getElementById('devModeEnabled').checked = settings.devModeEnabled || false;
                document.getElementById('overrideMode').value = settings.overrideMode || 0;
            } catch (error) {
                console.error('Error loading settings:', error);
            }
        };
        
        // Add WiFi scanning functionality
        function scanNetworks() {
            const networksList = document.getElementById('networks-list');
            networksList.innerHTML = '<div class="loading">Scanning...</div>';
            
            fetch('/scan')
                .then(response => response.json())
                .then(networks => {
                    networksList.innerHTML = '';
                    networks.forEach(network => {
                        const div = document.createElement('div');
                        div.className = 'network-item';
                        div.innerHTML = `${network.ssid} (${network.rssi}dBm) ${network.secure ? '🔒' : ''}`;
                        div.onclick = () => {
                            document.getElementById('ssid').value = network.ssid;
                            document.getElementById('password').focus();
                        };
                        networksList.appendChild(div);
                    });
                })
                .catch(error => {
                    networksList.innerHTML = '<div>Error scanning networks</div>';
                });
        }

       // Form submission
        document.getElementById('settingsForm').onsubmit = function(e) {
            e.preventDefault();
            
            // Create a status div if it doesn't exist
            let statusDiv = document.getElementById('statusDiv');
            if (!statusDiv) {
                statusDiv = document.createElement('div');
                statusDiv.id = 'statusDiv';
                statusDiv.style.padding = '10px';
                statusDiv.style.margin = '10px 0';
                statusDiv.style.border = '1px solid #ccc';
                statusDiv.style.borderRadius = '4px';
                document.getElementById('settingsForm').appendChild(statusDiv);
            }
            
            statusDiv.innerHTML = 'Sending request...';
            statusDiv.style.backgroundColor = '#f0f0f0';
            
            // Gather form data
            const formData = {
                latitude: parseFloat(document.getElementById('latitude').value),
                longitude: parseFloat(document.getElementById('longitude').value),
                calculationMethod: parseInt(document.getElementById('method').value),
                school: parseInt(document.getElementById('school').value),
                devModeEnabled: document.getElementById('devModeEnabled').checked,
                overrideMode: parseInt(document.getElementById('overrideMode').value),
                ssid: document.getElementById('ssid').value,
                password: document.getElementById('password').value
            };
            
            // Log to both screen and console
            statusDiv.innerHTML += '<br>Submitting data: ' + JSON.stringify(formData);
            console.log('Submitting data:', formData);
            
            // Use XMLHttpRequest for better compatibility
            const xhr = new XMLHttpRequest();
            
            // Setup request
            xhr.open('POST', '/settings', true);
            xhr.setRequestHeader('Content-Type', 'application/json');
            xhr.timeout = 15000; // 15 seconds timeout
            
            // Define handlers
            xhr.onreadystatechange = function() {
                statusDiv.innerHTML += '<br>Ready state changed: ' + xhr.readyState;
                
                if (xhr.readyState === 4) {
                    statusDiv.innerHTML += '<br>Status: ' + xhr.status;
                    
                    if (xhr.status >= 200 && xhr.status < 300) {
                        statusDiv.innerHTML += '<br><strong>Settings saved successfully!</strong>';
                        statusDiv.innerHTML += '<br>Device will restart and connect to the new WiFi network.';
                        statusDiv.style.backgroundColor = '#d4edda';
                        
                        // Disable the form
                        document.querySelectorAll('#settingsForm input, #settingsForm select, #settingsForm button').forEach(element => {
                            element.disabled = true;
                        });
                        
                        // Start a countdown
                        let seconds = 15;
                        const countdownElem = document.createElement('div');
                        countdownElem.innerHTML = 'Please connect to your WiFi network in ' + seconds + ' seconds...';
                        statusDiv.appendChild(countdownElem);
                        
                        const interval = setInterval(() => {
                            seconds--;
                            countdownElem.innerHTML = 'Please connect to your WiFi network in ' + seconds + ' seconds...';
                            if (seconds <= 0) {
                                clearInterval(interval);
                                countdownElem.innerHTML = 'Device has restarted. You can close this page now.';
                            }
                        }, 1000);
                    } else {
                        statusDiv.innerHTML += '<br><strong>Error:</strong> ' + (xhr.responseText || 'Unknown error');
                        statusDiv.style.backgroundColor = '#f8d7da';
                    }
                }
            };
            
            xhr.ontimeout = function() {
                statusDiv.innerHTML += '<br><strong>Request timed out</strong>';
                statusDiv.innerHTML += '<br>This might mean the device is restarting.';
                statusDiv.style.backgroundColor = '#fff3cd';
            };
            
            xhr.onerror = function() {
                statusDiv.innerHTML += '<br><strong>Network error occurred</strong>';
                statusDiv.innerHTML += '<br>The device might be restarting.';
                statusDiv.style.backgroundColor = '#fff3cd';
            };
            
            // Send the request
            try {
                const jsonData = JSON.stringify(formData);
                statusDiv.innerHTML += '<br>Sending JSON: ' + jsonData;
                xhr.send(jsonData);
            } catch (error) {
                statusDiv.innerHTML += '<br><strong>Error sending request:</strong> ' + error.message;
                statusDiv.style.backgroundColor = '#f8d7da';
            }
            
            return false;
        };


        
    </script>
</body>
</html>
)rawliteral";


//=============================================================================================================
SettingsServer::SettingsServer() : server(80) {}

void SettingsServer::begin() {
    preferences.begin("prayerTimes", false);
    loadSettings();

    // Try to connect with saved credentials first
    if (strlen(settings.ssid) > 0) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(settings.ssid, settings.password);
        
        // Wait up to 10 seconds for connection
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        Serial.println("");
    }


    // If connection failed, decide whether to enter AP mode or reconnection mode
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n====================");
        Serial.println("WiFi connection failed");
        
        if (shouldEnterAPMode()) {
            Serial.println("Starting AP mode");
            Serial.println("Connect to 'PrayerDisplay_Setup' WiFi network");
            Serial.println("Then navigate to: 192.168.4.1");
            Serial.println("====================\n");
            startAPMode();
            
            if (_display && _renderer) {
                renderAPModeInstructions();
            }
        } else {
            Serial.println("Starting reconnection attempts");
            Serial.println("====================\n");
            // Reconnection screen is handled in startReconnectionTimer()
        }
    } else {
        Serial.println("\n====================");
        Serial.println("WiFi connected!");
        Serial.println("Settings portal available at: " + WiFi.localIP().toString());
        Serial.println("====================\n");
    }
    /*
    // If connection failed, start AP mode
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n====================");
        Serial.println("WiFi connection failed, starting AP mode");
        Serial.println("Connect to 'PrayerDisplay_Setup' WiFi network");
        Serial.println("Then navigate to: 192.168.4.1");
        Serial.println("====================\n");
        startAPMode();
        if (_display && _renderer) {
            Serial.println("Rendering AP mode instructions to display");
            renderAPModeInstructions();
        }
    } else {
        Serial.println("\n====================");
        Serial.println("WiFi connected!");
        Serial.println("Settings portal available at: " + WiFi.localIP().toString());
        Serial.println("====================\n");
    }
    */

    // Set up web server routes
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/settings", HTTP_GET, [this]() { handleGetSettings(); });
    server.on("/settings", HTTP_POST, [this]() { handleSetSettings(); });
    server.on("/scan", HTTP_GET, [this]() { handleWiFiScan(); });
    
    // Handle captive portal in AP mode
    server.onNotFound([this]() {
        if (isInAPMode) {
            // For captive portal, redirect all requests to the root page
            Serial.print("Redirecting request to root: ");
            Serial.println(server.uri());
            
            // Apple devices often request this file to detect captive portals
            if (server.uri().endsWith(".html") || 
                server.uri().indexOf("generate_204") >= 0 ||
                server.uri().indexOf("redirect") >= 0 ||
                server.uri().indexOf("hotspot-detect") >= 0) {
                server.sendHeader("Location", "http://192.168.4.1/", true);
                server.send(302, "text/plain", "");
            } else {
                handleRoot();
            }
        } else {
            server.send(404, "text/plain", "Not found");
        }
    });

    

    server.begin();
    Serial.println("HTTP server started");
}

void SettingsServer::handle() {
    if (isInAPMode) {
        dnsServer.processNextRequest();
        
        // Periodically attempt to reconnect to WiFi even in AP mode
        unsigned long currentTime = millis();
        if (strlen(settings.ssid) > 0 && currentTime - _lastAPModeReconnectAttempt >= AP_MODE_RECHECK_INTERVAL) {
            _lastAPModeReconnectAttempt = currentTime;
            
            Serial.println("AP Mode: Attempting to reconnect to WiFi...");
            
            // Quick attempt to reconnect
            WiFi.disconnect();
            WiFi.mode(WIFI_STA);
            WiFi.begin(settings.ssid, settings.password);
            
            // Short wait for connection (5 seconds max)
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 10) {
                delay(500);
                Serial.print(".");
                attempts++;
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                // Successfully reconnected
                Serial.println("\nWiFi reconnected from AP mode! Restarting device...");
                isInAPMode = false;
                
                // Restart to apply normal operation
                delay(1000);
                ESP.restart();
            } else {
                // Failed to reconnect, stay in AP mode
                Serial.println("\nStill unable to connect to WiFi, remaining in AP mode.");
                WiFi.mode(WIFI_AP);
            }
        }
    } else if (_reconnectionMode) {
        checkReconnection();
    }
    server.handleClient();
}

void SettingsServer::loadSettings() {
    // Existing settings stay the same
    // !! Should probbaly amend this at some point to have agnostic default settings
    //SYDNEY
    settings.latitude = preferences.getFloat("lat", -33.8688);
    settings.longitude = preferences.getFloat("lon", 151.2093);

    //LEETON
    //settings.latitude = preferences.getFloat("lat", -34.5403);
    //settings.longitude = preferences.getFloat("lon", 146.4030);
    

    settings.calculationMethod = preferences.getInt("method", 3);
    settings.school = preferences.getInt("school", 1);
    settings.devModeEnabled = preferences.getBool("devMode", false);
    settings.overrideMode = preferences.getInt("override", 0);
    
    // WiFi settings
    static const int SSID_MAX_LENGTH = 33;
    static const int PASSWORD_MAX_LENGTH = 64;

    // First, check if there are stored WiFi credentials in preferences
    size_t ssidLen = preferences.getString("ssid", settings.ssid, SSID_MAX_LENGTH);
    
    // For first-time use or if preferences are empty
    if (ssidLen == 0) {
        // Clear the strings just to be safe
        settings.ssid[0] = '\0';
        settings.password[0] = '\0';
        
        // If in development mode and hardcoded WiFi is enabled, use those credentials
        if (USE_HARDCODED_WIFI) {
            Serial.println("DEV MODE: Using hardcoded WiFi credentials");
            strncpy(settings.ssid, WIFI_SSID, static_cast<size_t>(SSID_MAX_LENGTH - 1));
            settings.ssid[SSID_MAX_LENGTH - 1] = '\0';
            strncpy(settings.password, WIFI_PASSWORD, static_cast<size_t>(PASSWORD_MAX_LENGTH - 1));
            settings.password[PASSWORD_MAX_LENGTH - 1] = '\0';
            
            if (SAVE_HARDCODED_TO_PREFERENCES) {
                preferences.putString("ssid", settings.ssid);
                preferences.putString("pass", settings.password);
            }
        } else {
            Serial.println("No stored WiFi credentials - device will enter AP mode");
        }
    } else {
        // If we have an SSID in preferences, get the password too
        preferences.getString("pass", settings.password, PASSWORD_MAX_LENGTH);
        Serial.println("Loaded stored WiFi credentials");
    }

    Serial.printf("WiFi Settings loaded - SSID: %s\n", settings.ssid);
}

void SettingsServer::saveSettings() {
    // Make sure preferences is opened
    if (!preferences.begin("prayerTimes", false)) {
        Serial.println("Failed to open preferences");
        return;
    }

    Serial.println("Saving settings to preferences...");
    
    // Save all settings
    preferences.putFloat("lat", settings.latitude);
    preferences.putFloat("lon", settings.longitude);
    preferences.putInt("method", settings.calculationMethod);
    preferences.putInt("school", settings.school);
    preferences.putBool("devMode", settings.devModeEnabled);
    preferences.putInt("override", settings.overrideMode);
    
    // Save WiFi credentials
    if (strlen(settings.ssid) > 0) {
        preferences.putString("ssid", settings.ssid);
        preferences.putString("pass", settings.password);
        
        Serial.print("Saved SSID: ");
        Serial.println(settings.ssid);
    }
    
    preferences.end();
    Serial.println("Settings saved successfully");
}

void SettingsServer::handleSetSettings() {
    Serial.println("handleSetSettings called");
    
    // Check if we're processing a POST request
    if (server.method() != HTTP_POST) {
        server.send(405, "text/plain", "Method Not Allowed");
        Serial.println("Error: Method not allowed");
        return;
    }
    
    // Check for content type
    if (server.hasHeader("Content-Type")) {
        Serial.print("Content-Type: ");
        Serial.println(server.header("Content-Type"));
    } else {
        Serial.println("No Content-Type header");
    }
    
    // Check if we have plain data
    if (server.hasArg("plain")) {
        String plainData = server.arg("plain");
        Serial.print("Received data: ");
        Serial.println(plainData);
        
        // Parse JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, plainData);
        
        if (error) {
            Serial.print("JSON parsing failed: ");
            Serial.println(error.c_str());
            server.send(400, "text/plain", String("JSON parsing failed: ") + error.c_str());
            return;
        }
        
        // Process settings
        Serial.println("Processing settings...");
        
        // Handle basic settings
        if (doc.containsKey("latitude")) settings.latitude = doc["latitude"].as<float>();
        if (doc.containsKey("longitude")) settings.longitude = doc["longitude"].as<float>();
        if (doc.containsKey("calculationMethod")) settings.calculationMethod = doc["calculationMethod"].as<int>();
        if (doc.containsKey("school")) settings.school = doc["school"].as<int>();
        if (doc.containsKey("devModeEnabled")) settings.devModeEnabled = doc["devModeEnabled"].as<bool>();
        if (doc.containsKey("overrideMode")) settings.overrideMode = doc["overrideMode"].as<int>();
        
        // Handle WiFi settings
        bool wifiChanged = false;
        if (doc.containsKey("ssid") && strlen(doc["ssid"]) > 0) {
            const char* newSsid = doc["ssid"];
            strlcpy(settings.ssid, newSsid, sizeof(settings.ssid));
            
            if (doc.containsKey("password")) {
                const char* newPassword = doc["password"];
                strlcpy(settings.password, newPassword, sizeof(settings.password));
            }
            
            wifiChanged = true;
            Serial.print("WiFi SSID set to: ");
            Serial.println(settings.ssid);
        }
        
        // Save settings
        saveSettings();
        Serial.println("Settings saved to preferences");
        
        // Send response
        server.send(200, "text/plain", "Settings saved successfully. Device will restart if WiFi changed.");
        Serial.println("Response sent");
        
        // If WiFi settings changed, restart the device
        if (wifiChanged) {
            Serial.println("WiFi settings changed. Restarting in 2 seconds...");
            delay(2000); // Give time for the response to be sent
            ESP.restart();
        }
    } else {
        Serial.println("No data received");
        server.send(400, "text/plain", "No data received");
    }
}

void SettingsServer::handleRoot() {
    server.send(200, "text/html", CONFIG_PAGE);
}

void SettingsServer::handleGetSettings() {
    String response;
    JsonDocument doc;
    
    doc["latitude"] = settings.latitude;
    doc["longitude"] = settings.longitude;
    doc["calculationMethod"] = settings.calculationMethod;
    doc["school"] = settings.school;
    doc["devModeEnabled"] = settings.devModeEnabled;
    doc["overrideMode"] = settings.overrideMode;
    doc["ssid"] = settings.ssid;
    
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void SettingsServer::handleWiFiScan() {
    String json = "[";
    int n = WiFi.scanComplete();
    if (n == -2) {
        WiFi.scanNetworks(true);
        server.send(200, "application/json", "[]");
        return;
    } else if (n) {
        for (int i = 0; i < n; ++i) {
            if (i) json += ",";
            json += "{";
            json += "\"ssid\":\"" + WiFi.SSID(i) + "\"";
            json += ",\"rssi\":" + String(WiFi.RSSI(i));
            json += ",\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
            json += "}";
        }
        WiFi.scanDelete();
        if (WiFi.scanComplete() == -2) {
            WiFi.scanNetworks(true);
        }
    }
    json += "]";
    server.send(200, "application/json", json);
}

void SettingsServer::setDisplay(GxEPD2_GFX* display, Renderer* renderer) {
    _display = display;
    _renderer = renderer;
}

void SettingsServer::setRenderArea(int16_t x, int16_t y, int16_t w, int16_t h) {
    _renderX = x;
    _renderY = y;
    _renderWidth = w;
    _renderHeight = h;
}

// render AP mode instructions

void SettingsServer::renderAPModeInstructions() {
    if (!_display || !_renderer) {
        Serial.println("Display or renderer not set for AP mode instructions");
        return;
    }

    // Initialize display for full refresh
    _display->setFullWindow();
    _display->firstPage();
    
    do {
        _display->fillScreen(GxEPD_WHITE);
        
        // Calculate layout positions - with Y_MARGIN
        int16_t centerX = _display->width() / 2;
        int16_t titleY = Y_MARGIN + 40;  // Start below Y_MARGIN
        int16_t lineHeight = 32;
        int16_t contentStartY = titleY + 60;
        
        // Calculate positions for each section with better spacing
        int16_t step1Y = contentStartY;
        int16_t step2Y = step1Y + lineHeight * 2;
        int16_t step3Y = step2Y + lineHeight * 2;
        int16_t step4Y = step3Y + lineHeight * 2;
        
        // Draw title
        _display->setTextColor(GxEPD_BLACK);
        _display->setFont(&FreeSansBold18pt7b);
        _renderer->drawString(centerX, titleY, "WiFi Setup Mode", CENTER);
        
        // Draw decorative underline
        _display->drawFastHLine(centerX - 150, titleY + 15, 300, GxEPD_BLACK);
        
        // Step 1: Connect to WiFi network
        _display->setFont(&FreeSansBold12pt7b);
        _renderer->drawString(centerX, step1Y, "1. Connect to WiFi:", CENTER);
        
        _display->setFont(&FreeSansBold18pt7b);
        _renderer->drawString(centerX, step1Y + lineHeight, AP_SSID, CENTER);
        
        // Step 2: Password
        _display->setFont(&FreeSansBold12pt7b);
        _renderer->drawString(centerX, step2Y, "2. Password:", CENTER);
        
        _display->setFont(&FreeSansBold18pt7b);
        _renderer->drawString(centerX, step2Y + lineHeight, AP_PASSWORD, CENTER);
        
        // Step 3: Browser
        _display->setFont(&FreeSansBold12pt7b);
        _renderer->drawString(centerX, step3Y, "3. Open browser, go to:", CENTER);
        
        _display->setFont(&FreeSansBold18pt7b);
        _renderer->drawString(centerX, step3Y + lineHeight, "192.168.4.1", CENTER);
        
        // Step 4: Configure
        _display->setFont(&FreeSansBold12pt7b);
        int16_t finalInstructionsY = step4Y;
        _renderer->drawString(centerX, finalInstructionsY, "4. Configure settings & save", CENTER);
        
        // Draw decorative border
        _display->drawRoundRect(30, Y_MARGIN - 20, _display->width() - 60, _display->height() - (Y_MARGIN * 2) + 40, 15, GxEPD_BLACK);
        
        // Draw WiFi symbol
        int16_t wifiX = 60;
        int16_t wifiY = titleY - 5;
        
        // Base circle
        _display->fillCircle(wifiX, wifiY, 6, GxEPD_BLACK);
        
        // Signal arcs
        for (int i = 0; i < 3; i++) {
            int rad = 12 + (i * 8);
            for (int angle = -60; angle <= 60; angle += 15) {
                float radians = angle * PI / 180.0;
                int x1 = wifiX + rad * cos(radians);
                int y1 = wifiY + rad * sin(radians);
                int x2 = wifiX + rad * cos((angle+15) * PI / 180.0);
                int y2 = wifiY + rad * sin((angle+15) * PI / 180.0);
                _display->drawLine(x1, y1, x2, y2, GxEPD_BLACK);
            }
        }
        
    } while (_display->nextPage());
}

bool SettingsServer::shouldEnterAPMode() {
    // Only enter AP mode if:
    // 1. No wifi credentials are stored, OR
    // 2. We've exceeded maximum reconnect attempts
    if (strlen(settings.ssid) == 0 || _reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
        return true;
    }
    
    // Otherwise, start reconnection mode
    if (!_reconnectionMode) {
        _reconnectionMode = true;
        _reconnectAttempts = 0;
        _lastReconnectAttempt = 0; // Force immediate reconnect attempt
        startReconnectionTimer();
    }
    
    return false;
}

void SettingsServer::startReconnectionTimer() {
    _lastReconnectAttempt = millis();
    renderReconnectingScreen();
}

void SettingsServer::checkReconnection() {
    if (!_reconnectionMode) return;
    
    unsigned long currentTime = millis();
    
    // Check if it's time for another reconnect attempt
    if (currentTime - _lastReconnectAttempt >= RECONNECT_INTERVAL) {
        _reconnectAttempts++;
        
        Serial.printf("WiFi reconnection attempt %d of %d\n", 
                    _reconnectAttempts, MAX_RECONNECT_ATTEMPTS);
        
        // Attempt to reconnect
        WiFi.disconnect();
        WiFi.begin(settings.ssid, settings.password);
        
        // Wait up to 10 seconds for connection
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            // Successfully reconnected
            Serial.println("\nWiFi reconnected!");
            _reconnectionMode = false;
            _reconnectAttempts = 0;
            
            // Restart to apply normal operation
            delay(1000);
            ESP.restart();
        } else {
            // Failed to reconnect, update the display and try again later
            _lastReconnectAttempt = currentTime;
            renderReconnectingScreen();
            
            // If we've reached max attempts, enter AP mode
            if (_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
                Serial.println("\nMax reconnection attempts reached. Entering AP mode.");
                startAPMode();
                renderAPModeInstructions();
            }
        }
    }
}

void SettingsServer::renderReconnectingScreen() {
    if (!_display || !_renderer) {
        Serial.println("Display or renderer not set for reconnection screen");
        return;
    }

    // Initialize display for full refresh
    _display->setFullWindow();
    _display->firstPage();
    
    do {
        _display->fillScreen(GxEPD_WHITE);
        
        // Calculate layout positions - with Y_MARGIN
        int16_t centerX = _display->width() / 2;
        int16_t titleY = Y_MARGIN + 40;  // Start below Y_MARGIN
        int16_t statusY = titleY + 60;
        int16_t detailsY = statusY + 50;
        int16_t countdownY = detailsY + 80;
        
        // Draw title
        _display->setTextColor(GxEPD_BLACK);
        _display->setFont(&FreeSansBold18pt7b);
        _renderer->drawString(centerX, titleY, "WiFi Connection Lost", CENTER);
        
        // Draw status
        _display->setFont(&FreeSansBold12pt7b);
        _renderer->drawString(centerX, statusY, 
            "Attempting to reconnect to:", CENTER);
            
        _display->setFont(&FreeSansBold18pt7b);
        _renderer->drawString(centerX, statusY + 40, settings.ssid, CENTER);
        
        // Draw attempt counter
        _display->setFont(&FreeSans9pt7b);
        String attemptText = "Attempt " + String(_reconnectAttempts) + 
                            " of " + String(MAX_RECONNECT_ATTEMPTS);
        _renderer->drawString(centerX, detailsY, attemptText, CENTER);
        
        // Draw next attempt countdown
        int secondsRemaining = 
            (RECONNECT_INTERVAL - (millis() - _lastReconnectAttempt)) / 1000;
        if (secondsRemaining < 0) secondsRemaining = 0;
        
        String countdownText;
        if (_reconnectAttempts == 0) {
            countdownText = "Connecting now...";
        } else {
            countdownText = "Next attempt in " + String(secondsRemaining) + " seconds";
        }
        _renderer->drawString(centerX, countdownY, countdownText, CENTER);
        
        // Draw info about AP mode
        if (_reconnectAttempts > MAX_RECONNECT_ATTEMPTS / 2) {
            String apModeText = "After " + String(MAX_RECONNECT_ATTEMPTS) + 
                               " attempts, setup mode will activate";
            _renderer->drawString(centerX, countdownY + 40, apModeText, CENTER);
        }
        
        // Draw decorative border
        _display->drawRoundRect(30, Y_MARGIN - 20, _display->width() - 60, _display->height() - (Y_MARGIN * 2) + 40, 15, GxEPD_BLACK);
        
        // Draw WiFi icon with "?" to indicate connectivity issue
        // Center circle
        _display->fillCircle(centerX - 120, titleY - 5, 5, GxEPD_BLACK);
        
        // Signal arcs - draw them dashed to indicate connectivity issues
        for (int i = 0; i < 3; i++) {
            int rad = 10 + (i * 5);
            for (int angle = -60; angle <= 60; angle += 30) {
                float radians = angle * PI / 180.0;
                int x1 = centerX - 120 + rad * cos(radians);
                int y1 = titleY - 5 + rad * sin(radians);
                int x2 = centerX - 120 + rad * cos((angle+15) * PI / 180.0);
                int y2 = titleY - 5 + rad * sin((angle+15) * PI / 180.0);
                _display->drawLine(x1, y1, x2, y2, GxEPD_BLACK);
            }
        }
        
        // Draw question mark
        _display->setFont(&FreeSansBold12pt7b);
        _renderer->drawString(centerX - 120 + 25, titleY - 5, "?", LEFT);
        
    } while (_display->nextPage());
}

bool SettingsServer::startAPMode() {
    // Disconnect from any existing WiFi
    WiFi.disconnect();
    delay(100);
    
    // Set WiFi mode to Access Point
    WiFi.mode(WIFI_AP);
    delay(100);
    
    // Configure AP with fixed IP
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), 
                      IPAddress(192, 168, 4, 1), 
                      IPAddress(255, 255, 255, 0));
    
    // Start the AP
    bool success = WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    if (success) {
        // Start DNS server for captive portal
        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.start(DNS_PORT, "*", IPAddress(192, 168, 4, 1));
        
        isInAPMode = true;
        Serial.println("AP Mode started successfully");
        Serial.print("AP IP address: ");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("AP Mode failed to start");
    }
    
    return success;
}

