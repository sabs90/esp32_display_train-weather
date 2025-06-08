#include "settings_server.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "config.h"

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
        .debug-output { padding: 10px; border: 1px solid #ddd; border-radius: 4px; margin-top: 20px; }
        .status { padding: 10px; margin: 10px 0; border: 1px solid #ccc; border-radius: 4px; }
    </style>
</head>
<body>
    <div class="container">
        <h2>Prayer Times Settings</h2>
        <div id="status" class="status" style="display: none;"></div>
        
        <div class="field">
            <label for="latitude">Latitude:</label>
            <input type="number" id="latitude" step="0.0001" value="-33.8688">
        </div>
        <div class="field">
            <label for="longitude">Longitude:</label>
            <input type="number" id="longitude" step="0.0001" value="151.2093">
        </div>
        <div class="field">
            <label for="method">Calculation Method:</label>
            <select id="method">
                <option value="0">Shia Ithna-Ashari</option>
                <option value="1">University of Islamic Sciences, Karachi</option>
                <option value="2">Islamic Society of North America</option>
                <option value="3" selected>Muslim World League</option>
                <option value="4">Umm Al-Qura University, Makkah</option>
                <option value="5">Egyptian General Authority of Survey</option>
            </select>
        </div>
        <div class="field">
            <label for="school">School:</label>
            <select id="school">
                <option value="0">Shafi</option>
                <option value="1" selected>Hanafi</option>
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
                    <option value="0" selected>Normal</option>
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
                <input type="text" id="ssid" placeholder="Enter WiFi network name">
                <button type="button" onclick="scanNetworks()" style="margin-bottom: 10px;">Scan Networks</button>
                <div id="networks-list" class="networks-list"></div>
            </div>
            <div class="field">
                <label for="password">Password:</label>
                <input type="password" id="password" placeholder="Enter WiFi password">
            </div>
        </div>

        <button id="saveBtn" onclick="saveSettings()" style="margin-top: 20px;">Save Settings</button>
    </div>

    <script>
        console.log('Script starting...');

        // Simple save function with inline onclick
        function saveSettings() {
            console.log('saveSettings() called directly');
            
            const btn = document.getElementById('saveBtn');
            const status = document.getElementById('status');
            
            // Update button immediately
            btn.innerHTML = '⏳ Saving...';
            btn.style.backgroundColor = '#ffc107';
            btn.disabled = true;
            
            // Show status
            status.style.display = 'block';
            status.style.background = '#fff3cd';
            status.innerHTML = '💾 Saving settings...';
            
            // Get values
            const data = {
                ssid: document.getElementById('ssid').value,
                password: document.getElementById('password').value,
                latitude: document.getElementById('latitude').value,
                longitude: document.getElementById('longitude').value,
                calculationMethod: document.getElementById('method').value,
                school: document.getElementById('school').value,
                devModeEnabled: document.getElementById('devModeEnabled').checked,
                overrideMode: document.getElementById('overrideMode').value
            };
            
            console.log('Data to send:', data);
            
            // Create URL encoded form data (most compatible)
            const params = new URLSearchParams();
            params.append('ssid', data.ssid);
            params.append('password', data.password);
            params.append('latitude', data.latitude);
            params.append('longitude', data.longitude);
            params.append('calculationMethod', data.calculationMethod);
            params.append('school', data.school);
            params.append('devModeEnabled', data.devModeEnabled ? 'true' : 'false');
            params.append('overrideMode', data.overrideMode);
            
            console.log('Sending POST to /settings');
            
            fetch('/settings', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: params.toString()
            })
            .then(response => {
                console.log('Response:', response.status, response.statusText);
                status.innerHTML = 'Response: ' + response.status + ' ' + response.statusText;
                
                if (!response.ok) {
                    throw new Error('HTTP ' + response.status);
                }
                return response.text();
            })
            .then(result => {
                console.log('Success:', result);
                status.innerHTML = '✅ ' + result;
                status.style.background = '#d4edda';
                
                if (data.ssid) {
                    setTimeout(() => {
                        status.innerHTML += '<br>🔄 Device restarting...';
                    }, 1000);
                }
            })
            .catch(error => {
                console.error('Error:', error);
                status.innerHTML = '❌ Error: ' + error.message;
                status.style.background = '#f8d7da';
                
                // Reset button
                btn.disabled = false;
                btn.innerHTML = 'Save Settings';
                btn.style.backgroundColor = '#4CAF50';
            });
        }

        // Load current settings
        function loadSettings() {
            console.log('Loading current settings...');
            fetch('/settings')
                .then(response => response.json())
                .then(settings => {
                    console.log('Settings loaded:', settings);
                    document.getElementById('latitude').value = settings.latitude || -33.8688;
                    document.getElementById('longitude').value = settings.longitude || 151.2093;
                    document.getElementById('method').value = settings.calculationMethod || 3;
                    document.getElementById('school').value = settings.school || 1;
                    document.getElementById('ssid').value = settings.ssid || '';
                    document.getElementById('devModeEnabled').checked = settings.devModeEnabled || false;
                    document.getElementById('overrideMode').value = settings.overrideMode || 0;
                })
                .catch(error => {
                    console.error('Error loading settings:', error);
                });
        }

        // WiFi scan
        function scanNetworks() {
            console.log('Scanning networks...');
            const list = document.getElementById('networks-list');
            list.innerHTML = '<div class="loading">🔍 Scanning...</div>';
            
            fetch('/scan')
                .then(response => response.json())
                .then(networks => {
                    console.log('Found', networks.length, 'networks');
                    list.innerHTML = '';
                    networks.forEach(network => {
                        const div = document.createElement('div');
                        div.className = 'network-item';
                        div.innerHTML = network.ssid + ' (' + network.rssi + 'dBm) ' + (network.secure ? '🔒' : '');
                        div.onclick = () => {
                            document.getElementById('ssid').value = network.ssid;
                            document.getElementById('password').focus();
                        };
                        list.appendChild(div);
                    });
                })
                .catch(error => {
                    console.error('Scan error:', error);
                    list.innerHTML = '<div>Error: ' + error.message + '</div>';
                });
        }

        // Load settings when page loads
        window.onload = loadSettings;
        
        console.log('Script loaded successfully');
    </script>
</body>
</html>
)rawliteral";


//=============================================================================================================
SettingsServer::SettingsServer() : server(80) {}

void SettingsServer::begin() {
    Serial.println("\n🚀 === SettingsServer::begin() STARTING ===");
    Serial.printf("📊 Free heap at start: %d bytes\n", ESP.getFreeHeap());
    
    preferences.begin("prayerTimes", false);
    Serial.println("✅ Preferences opened successfully");
    
    loadSettings();

    Serial.printf("📡 Loaded SSID: '%s' (length: %d)\n", settings.ssid, strlen(settings.ssid));
    Serial.printf("🔒 Password length: %d characters\n", strlen(settings.password));
    Serial.printf("🌍 Location: %.6f, %.6f\n", settings.latitude, settings.longitude);
    Serial.printf("📖 Method: %d, School: %d\n", settings.calculationMethod, settings.school);

    // Try to connect with saved credentials first
    if (strlen(settings.ssid) > 0) {
        Serial.printf("🔄 Attempting to connect to '%s'...\n", settings.ssid);
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
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("✅ WiFi connected with stored credentials!");
            Serial.printf("🌐 IP Address: %s\n", WiFi.localIP().toString().c_str());
        } else {
            Serial.printf("❌ Failed to connect to '%s'\n", settings.ssid);
        }
    } else {
        Serial.println("⚠️  No stored WiFi credentials found");
    }

    // If connection failed, decide whether to enter AP mode or reconnection mode
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n====================");
        Serial.println("WiFi connection failed");
        
        if (shouldEnterAPMode()) {
            Serial.println("🔧 Starting AP mode");
            Serial.println("📶 Connect to 'PrayerDisplay_Setup' WiFi network");
            Serial.println("🌐 Then navigate to: 192.168.4.1");
            Serial.println("====================\n");
            
            if (startAPMode()) {
                if (_display && _renderer) {
                    renderAPModeInstructions();
                }
            } else {
                Serial.println("❌ ERROR: Failed to start AP mode!");
            }
        } else {
            Serial.println("🔄 Starting reconnection attempts");
            Serial.println("====================\n");
            startReconnectionTimer();
        }
    } else {
        Serial.println("\n====================");
        Serial.println("✅ WiFi connected!");
        Serial.printf("🌐 Settings portal available at: %s\n", WiFi.localIP().toString().c_str());
        Serial.println("====================\n");
    }

    // Set up web server routes with enhanced debugging
    Serial.println("🌐 Setting up web server routes...");
    
    server.on("/", HTTP_GET, [this]() { 
        Serial.println("🌐 DEBUG: Root page requested");
        handleRoot(); 
    });
    
    server.on("/settings", HTTP_GET, [this]() { 
        Serial.println("📖 DEBUG: GET /settings requested");
        handleGetSettings(); 
    });
    
    server.on("/settings", HTTP_POST, [this]() { 
        Serial.println("💾 DEBUG: POST /settings requested");
        handleSetSettings(); 
    });
    
    server.on("/scan", HTTP_GET, [this]() { 
        Serial.println("📡 DEBUG: WiFi scan requested");
        handleWiFiScan(); 
    });
    
    // Handle captive portal in AP mode - enhanced debugging
    server.onNotFound([this]() {
        Serial.printf("❓ DEBUG: Request not found - Method: %s, URI: %s\n", 
                     server.method() == HTTP_GET ? "GET" : 
                     server.method() == HTTP_POST ? "POST" : "OTHER", 
                     server.uri().c_str());
        
        if (isInAPMode) {
            // In AP mode, redirect everything to the main page for captive portal
            String redirectUrl = "http://192.168.4.1/";
            Serial.printf("🔄 Captive portal redirect: %s -> %s\n", server.uri().c_str(), redirectUrl.c_str());
            
            // Check if this is a POST request that should go to settings
            if (server.method() == HTTP_POST && server.uri() == "/settings") {
                Serial.println("💾 DEBUG: Redirecting POST /settings to handleSetSettings");
                handleSetSettings();
                return;
            }
            
            server.sendHeader("Location", redirectUrl, true);
            server.send(302, "text/plain", "");
        } else {
            server.send(404, "text/plain", "Not found");
        }
    });

    // Configure server to collect headers and enable body collection  
    const char* headerKeys[] = {"Content-Type", "Content-Length"};
    server.collectHeaders(headerKeys, 2);

    // Start the HTTP server
    server.begin();
    Serial.println("✅ HTTP server started");
    Serial.printf("📊 Free heap at end: %d bytes\n", ESP.getFreeHeap());
    Serial.println("🚀 === SettingsServer::begin() COMPLETE ===\n");
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
    Serial.println("📖 === loadSettings() STARTING ===");
    
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
    
    Serial.printf("📍 Loaded coordinates: %.6f, %.6f\n", settings.latitude, settings.longitude);
    Serial.printf("📋 Loaded method: %d, school: %d\n", settings.calculationMethod, settings.school);
    
    // WiFi settings
    static const int SSID_MAX_LENGTH = 33;
    static const int PASSWORD_MAX_LENGTH = 64;

    // First, check if there are stored WiFi credentials in preferences
    size_t ssidLen = preferences.getString("ssid", settings.ssid, SSID_MAX_LENGTH);
    
    Serial.printf("💾 Checking flash storage for WiFi credentials...\n");
    Serial.printf("💾 Found SSID in flash: '%s' (length: %d)\n", ssidLen > 0 ? settings.ssid : "(none)", ssidLen);
    
    // For first-time use or if preferences are empty
    if (ssidLen == 0) {
        Serial.println("⚠️  No WiFi credentials found in flash storage");
        
        // Clear the strings just to be safe
        settings.ssid[0] = '\0';
        settings.password[0] = '\0';
        
        // If in development mode and hardcoded WiFi is enabled, use those credentials
        if (USE_HARDCODED_WIFI) {
            Serial.println("🔧 DEV MODE: Using hardcoded WiFi credentials");
            Serial.printf("🔧 Hardcoded SSID: '%s'\n", WIFI_SSID);
            
            strncpy(settings.ssid, WIFI_SSID, static_cast<size_t>(SSID_MAX_LENGTH - 1));
            settings.ssid[SSID_MAX_LENGTH - 1] = '\0';
            strncpy(settings.password, WIFI_PASSWORD, static_cast<size_t>(PASSWORD_MAX_LENGTH - 1));
            settings.password[PASSWORD_MAX_LENGTH - 1] = '\0';
            
            if (SAVE_HARDCODED_TO_PREFERENCES) {
                Serial.println("💾 Saving hardcoded credentials to flash...");
                preferences.putString("ssid", settings.ssid);
                preferences.putString("pass", settings.password);
                Serial.println("✅ Hardcoded credentials saved to flash");
            } else {
                Serial.println("⚠️  Not saving hardcoded credentials to flash (SAVE_HARDCODED_TO_PREFERENCES=false)");
            }
        } else {
            Serial.println("🔧 No hardcoded WiFi enabled - device will enter AP mode");
        }
    } else {
        // If we have an SSID in preferences, get the password too
        size_t passLen = preferences.getString("pass", settings.password, PASSWORD_MAX_LENGTH);
        Serial.printf("💾 Found password in flash (length: %d)\n", passLen);
        Serial.println("✅ Loaded stored WiFi credentials from flash");
    }

    Serial.printf("📡 Final WiFi Settings - SSID: '%s' (length: %d)\n", settings.ssid, strlen(settings.ssid));
    Serial.printf("🔒 Final password length: %d characters\n", strlen(settings.password));
    Serial.println("📖 === loadSettings() COMPLETE ===");
}

void SettingsServer::saveSettings() {
    Serial.println("Saving settings to preferences...");
    
    // Preferences should already be opened in begin(), but let's ensure it's available
    // and close/reopen if needed for writing
    preferences.end();
    
    if (!preferences.begin("prayerTimes", false)) {
        Serial.println("ERROR: Failed to open preferences for writing!");
        return;
    }
    
    // Save all settings
    bool success = true;
    
    if (!preferences.putFloat("lat", settings.latitude)) {
        Serial.println("ERROR: Failed to save latitude");
        success = false;
    }
    if (!preferences.putFloat("lon", settings.longitude)) {
        Serial.println("ERROR: Failed to save longitude");
        success = false;
    }
    if (!preferences.putInt("method", settings.calculationMethod)) {
        Serial.println("ERROR: Failed to save calculation method");
        success = false;
    }
    if (!preferences.putInt("school", settings.school)) {
        Serial.println("ERROR: Failed to save school");
        success = false;
    }
    if (!preferences.putBool("devMode", settings.devModeEnabled)) {
        Serial.println("ERROR: Failed to save dev mode");
        success = false;
    }
    if (!preferences.putInt("override", settings.overrideMode)) {
        Serial.println("ERROR: Failed to save override mode");
        success = false;
    }
    
    // Save WiFi credentials
    if (strlen(settings.ssid) > 0) {
        if (!preferences.putString("ssid", settings.ssid)) {
            Serial.println("ERROR: Failed to save SSID");
            success = false;
        } else {
            Serial.printf("✓ Saved SSID: %s\n", settings.ssid);
        }
        
        if (!preferences.putString("pass", settings.password)) {
            Serial.println("ERROR: Failed to save password");
            success = false;
        } else {
            Serial.printf("✓ Saved password (length: %d)\n", strlen(settings.password));
        }
    }
    
    // Force commit changes to flash
    if (success) {
        preferences.end();
        Serial.println("✓ All settings saved successfully to flash");
        
        // Verify the save worked by reading it back
        if (preferences.begin("prayerTimes", true)) { // read-only mode
            String savedSsid = preferences.getString("ssid", "");
            Serial.printf("✓ Verification: SSID read back as: '%s'\n", savedSsid.c_str());
            preferences.end();
        }
    } else {
        preferences.end();
        Serial.println("❌ Some settings failed to save!");
    }
    
    // Reopen for future reads
    preferences.begin("prayerTimes", false);
}

void SettingsServer::handleSetSettings() {
    Serial.println("\n=== handleSetSettings() called ===");
    Serial.printf("Time: %lu ms\n", millis());
    Serial.printf("Method: %s\n", server.method() == HTTP_POST ? "POST" : 
                                  server.method() == HTTP_GET ? "GET" : "OTHER");
    Serial.printf("URI: %s\n", server.uri().c_str());
    Serial.printf("Args count: %d\n", server.args());
    
    // Print all arguments
    for (int i = 0; i < server.args(); i++) {
        Serial.printf("Arg[%d]: %s = %s\n", i, server.argName(i).c_str(), server.arg(i).c_str());
    }
    
    // Print all headers
    Serial.println("Headers:");
    for (int i = 0; i < server.headers(); i++) {
        Serial.printf("  %s: %s\n", server.headerName(i).c_str(), server.header(i).c_str());
    }
    
    // Check if we're processing a POST request
    if (server.method() != HTTP_POST) {
        Serial.println("ERROR: Method not allowed - expected POST");
        server.send(405, "text/plain", "Method Not Allowed");
        return;
    }
    
    // Get the request body - try multiple methods
    String requestBody = "";
    bool isFormData = false;
    
    // Check content type to determine how to parse the data
    String contentType = "";
    if (server.hasHeader("Content-Type")) {
        contentType = server.header("Content-Type");
        Serial.printf("Content-Type: %s\n", contentType.c_str());
        
        if (contentType.startsWith("application/x-www-form-urlencoded") || 
            contentType.startsWith("multipart/form-data")) {
            isFormData = true;
            Serial.println("Detected form data submission");
        } else if (contentType.startsWith("application/json")) {
            Serial.println("Detected JSON submission");
        }
    } else {
        Serial.println("WARNING: No Content-Type header found");
    }
    
    JsonDocument doc;
    
    if (isFormData || server.args() > 0) {
        // Handle form data submission (traditional forms)
        Serial.println("Processing as form data...");
        
        for (int i = 0; i < server.args(); i++) {
            String argName = server.argName(i);
            String argValue = server.arg(i);
            Serial.printf("Form field: %s = %s\n", argName.c_str(), argValue.c_str());
            
            // Convert form values to appropriate types for JSON document
            if (argName == "latitude" || argName == "longitude") {
                doc[argName] = argValue.toFloat();
            } else if (argName == "calculationMethod" || argName == "school" || argName == "overrideMode") {
                doc[argName] = argValue.toInt();
            } else if (argName == "devModeEnabled") {
                doc[argName] = (argValue == "true" || argValue == "on" || argValue == "1");
            } else {
                doc[argName] = argValue;
            }
        }
        
        String jsonString;
        serializeJson(doc, jsonString);
        Serial.printf("Converted form data to JSON: %s\n", jsonString.c_str());
        
    } else {
        // Handle JSON data submission
        Serial.println("Processing as JSON data...");
        
        // Method 1: Try to get from 'plain' argument (works for some content types)
        if (server.hasArg("plain")) {
            requestBody = server.arg("plain");
            Serial.printf("Got data from 'plain' argument: %s\n", requestBody.c_str());
        }
        
        if (requestBody.length() == 0) {
            Serial.println("ERROR: No JSON data found in request");
            server.send(400, "text/plain", "No JSON data received");
            return;
        }
        
        Serial.printf("Processing JSON body (length: %d): %s\n", requestBody.length(), requestBody.c_str());
        
        // Parse JSON
        DeserializationError error = deserializeJson(doc, requestBody);
        
        if (error) {
            Serial.printf("ERROR: JSON parsing failed: %s\n", error.c_str());
            Serial.printf("Raw data that failed to parse: '%s'\n", requestBody.c_str());
            String errorMsg = String("JSON parsing failed: ") + error.c_str();
            server.send(400, "text/plain", errorMsg);
            return;
        }
    }
    
    Serial.println("SUCCESS: JSON parsed successfully");
    serializeJsonPretty(doc, Serial);
    Serial.println();
    
    // Process settings
    Serial.println("Processing settings...");
    
    // Handle basic settings
    if (doc.containsKey("latitude")) {
        settings.latitude = doc["latitude"].as<float>();
        Serial.printf("✓ Latitude set to: %.6f\n", settings.latitude);
    }
    if (doc.containsKey("longitude")) {
        settings.longitude = doc["longitude"].as<float>();
        Serial.printf("✓ Longitude set to: %.6f\n", settings.longitude);
    }
    if (doc.containsKey("calculationMethod")) {
        settings.calculationMethod = doc["calculationMethod"].as<int>();
        Serial.printf("✓ Calculation method set to: %d\n", settings.calculationMethod);
    }
    if (doc.containsKey("school")) {
        settings.school = doc["school"].as<int>();
        Serial.printf("✓ School set to: %d\n", settings.school);
    }
    if (doc.containsKey("devModeEnabled")) {
        settings.devModeEnabled = doc["devModeEnabled"].as<bool>();
        Serial.printf("✓ Dev mode enabled: %s\n", settings.devModeEnabled ? "true" : "false");
    }
    if (doc.containsKey("overrideMode")) {
        settings.overrideMode = doc["overrideMode"].as<int>();
        Serial.printf("✓ Override mode set to: %d\n", settings.overrideMode);
    }
    
    // Handle WiFi settings with enhanced debugging
    bool wifiChanged = false;
    
    Serial.println("Processing WiFi settings...");
    
    if (doc.containsKey("ssid")) {
        const char* newSsid = doc["ssid"];
        Serial.printf("New SSID from request: '%s' (length: %d)\n", newSsid, strlen(newSsid));
        
        if (strlen(newSsid) > 0) {
            strlcpy(settings.ssid, newSsid, sizeof(settings.ssid));
            Serial.printf("✓ WiFi SSID changed from '%s' to '%s'\n", settings.ssid, newSsid);
            
            if (doc.containsKey("password")) {
                const char* newPassword = doc["password"];
                strlcpy(settings.password, newPassword, sizeof(settings.password));
                Serial.printf("✓ WiFi password updated (length: %d characters)\n", strlen(settings.password));
                
                // For security, don't print the actual password
                Serial.print("Password preview: ");
                if (strlen(settings.password) > 0) {
                    Serial.print(settings.password[0]);
                    for (int i = 1; i < strlen(settings.password); i++) {
                        Serial.print("*");
                    }
                }
                Serial.println();
            } else {
                Serial.println("WARNING: No password provided with SSID");
            }
            
            wifiChanged = true;
        } else {
            Serial.println("WARNING: Empty SSID provided, skipping WiFi settings");
        }
    } else {
        Serial.println("No SSID field found in request");
    }
    
    // Save settings
    Serial.println("Saving settings to preferences...");
    saveSettings();
    Serial.println("✓ Settings saved successfully to flash");
    
    // Send response
    String response = "Settings saved successfully";
    if (wifiChanged) {
        response += ". Device will restart to connect to new WiFi network.";
        Serial.println("WiFi settings have changed - device will restart after response");
    }
    
    Serial.printf("Sending HTTP response: %s\n", response.c_str());
    server.send(200, "text/plain", response);
    Serial.println("✓ HTTP response sent successfully");
    
    // If WiFi settings changed, restart the device
    if (wifiChanged) {
        Serial.println("\n🚨 *** WIFI CREDENTIALS CHANGED - RESTARTING DEVICE ***");
        Serial.printf("🆕 New SSID: '%s'\n", settings.ssid);
        Serial.printf("🔒 New password length: %d characters\n", strlen(settings.password));
        Serial.println("⏰ Device will restart in 3 seconds to apply new WiFi settings...");
        Serial.println("📋 After restart, device will attempt to connect to new network");
        
        for (int i = 3; i > 0; i--) {
            Serial.printf("⏳ Restart countdown: %d seconds remaining...\n", i);
            delay(1000);
        }
        
        Serial.println("🔄 Restarting ESP32 NOW...");
        Serial.println("===============================================");
        Serial.flush(); // Ensure all debug output is sent
        ESP.restart();
    }
    
    Serial.println("=== handleSetSettings() completed ===\n");
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
    Serial.println("WiFi scan requested");
    
    String json = "[";
    int n = WiFi.scanComplete();
    
    if (n == -2) {
        // Scan not triggered yet
        Serial.println("Starting WiFi scan...");
        WiFi.scanNetworks(true); // Async scan
        server.send(200, "application/json", "[]");
        return;
    } else if (n == -1) {
        // Scan in progress
        Serial.println("WiFi scan in progress...");
        server.send(200, "application/json", "[]");
        return;
    } else if (n == 0) {
        // No networks found
        Serial.println("No WiFi networks found");
        server.send(200, "application/json", "[]");
        return;
    } else {
        // Scan completed, process results
        Serial.printf("Found %d networks\n", n);
        for (int i = 0; i < n; ++i) {
            if (i) json += ",";
            json += "{";
            json += "\"ssid\":\"" + WiFi.SSID(i) + "\"";
            json += ",\"rssi\":" + String(WiFi.RSSI(i));
            json += ",\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
            json += "}";
        }
        WiFi.scanDelete();
        
        // Start a new scan for next time
        if (WiFi.scanComplete() == -2) {
            WiFi.scanNetworks(true);
        }
    }
    
    json += "]";
    Serial.printf("Sending scan results: %s\n", json.c_str());
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
    Serial.println("=== shouldEnterAPMode() ===");
    Serial.printf("SSID length: %d\n", strlen(settings.ssid));
    Serial.printf("Reconnect attempts: %d (max: %d)\n", _reconnectAttempts, MAX_RECONNECT_ATTEMPTS);
    
    // Only enter AP mode if:
    // 1. No wifi credentials are stored, OR
    // 2. We've exceeded maximum reconnect attempts
    if (strlen(settings.ssid) == 0) {
        Serial.println("Decision: Enter AP mode (no stored credentials)");
        return true;
    }
    
    if (_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
        Serial.println("Decision: Enter AP mode (max reconnect attempts exceeded)");
        return true;
    }
    
    // Otherwise, start reconnection mode
    if (!_reconnectionMode) {
        Serial.println("Decision: Enter reconnection mode");
        _reconnectionMode = true;
        _reconnectAttempts = 0;
        _lastReconnectAttempt = 0; // Force immediate reconnect attempt
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
    Serial.println("Starting AP Mode...");
    
    // Disconnect from any existing WiFi
    WiFi.disconnect();
    delay(500);
    
    // Set WiFi mode to Access Point
    WiFi.mode(WIFI_AP);
    delay(500);
    
    // Configure AP with fixed IP
    bool configSuccess = WiFi.softAPConfig(
        IPAddress(192, 168, 4, 1),    // AP IP
        IPAddress(192, 168, 4, 1),    // Gateway
        IPAddress(255, 255, 255, 0)   // Subnet mask
    );
    
    if (!configSuccess) {
        Serial.println("Failed to configure AP IP");
        return false;
    }
    
    // Start the AP with explicit parameters
    bool success = WiFi.softAP(AP_SSID, AP_PASSWORD, 1, 0, 4); // channel 1, hidden=false, max_connection=4
    
    if (success) {
        delay(2000); // Give AP time to fully start
        
        IPAddress IP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(IP);
        
        // Verify we can see the AP
        Serial.printf("AP SSID: %s\n", WiFi.softAPSSID().c_str());
        Serial.printf("Connected stations: %d\n", WiFi.softAPgetStationNum());
        
        // Start DNS server for captive portal
        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        bool dnsStarted = dnsServer.start(DNS_PORT, "*", IP);
        
        if (dnsStarted) {
            Serial.println("DNS server started for captive portal");
        } else {
            Serial.println("Warning: DNS server failed to start");
        }
        
        isInAPMode = true;
        Serial.println("AP Mode started successfully");
        Serial.println("Connect to 'PrayerDisplay_Setup' and navigate to 192.168.4.1");
    } else {
        Serial.println("AP Mode failed to start");
        // Try to restart WiFi
        WiFi.mode(WIFI_OFF);
        delay(1000);
    }
    
    return success;
}

