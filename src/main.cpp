/*
 * FluidDash v0.2.001 - CYD Edition with hard coded Screen Layouts
 * Configured for ESP32-2432S028 (CYD 3.5" or 4.0" modules)
 * - WiFiManager for initial setup
 * - Preferences for persistent storage
 * - Web interface for all settings
 * - Configurable graph timespan
 */

#include <Arduino.h>
#include "config/pins.h"
#include "config/config.h"
#include "display/display.h"
#include "display/screen_renderer.h"
#include "display/ui_modes.h"
#include "sensors/sensors.h"
#include "network/network.h"
#include "utils/utils.h"
#include <LovyanGFX.hpp>
#include <Wire.h>
#include <RTClib.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <WebSocketsClient.h>

#include <Preferences.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "storage_manager.h"

// Storage manager - handles SD/SPIFFS with fallback
StorageManager storage;

RTC_DS3231 rtc;
WebSocketsClient webSocket;
Preferences prefs;  // Needed for WiFi credentials storage
WebServer server(80);
WiFiManager wm;

// Runtime variables
DisplayMode currentMode;
bool sdCardAvailable = false;
volatile uint16_t tachCounter = 0;
uint16_t fanRPM = 0;
uint8_t fanSpeed = 0;
float temperatures[4] = {0};
float peakTemps[4] = {0};
float psuVoltage = 0;
float psuMin = 99.9;
float psuMax = 0.0;

// Non-blocking ADC sampling
uint32_t adcSamples[5][10];  // 4 thermistors + 1 PSU, 10 samples each
uint8_t adcSampleIndex = 0;
uint8_t adcCurrentSensor = 0;
unsigned long lastAdcSample = 0;
bool adcReady = false;

// Dynamic history buffer
float *tempHistory = nullptr;
uint16_t historySize = 0;
uint16_t historyIndex = 0;

// FluidNC status
String machineState = "OFFLINE";
float posX = 0, posY = 0, posZ = 0, posA = 0;
float wposX = 0, wposY = 0, wposZ = 0, wposA = 0;
int feedRate = 0;
int spindleRPM = 0;
bool fluidncConnected = false;
unsigned long jobStartTime = 0;
bool isJobRunning = false;

// ===== ADD NEW GLOBAL VARIABLES HERE =====
// Extended status fields
int feedOverride = 100;
int rapidOverride = 100;
int spindleOverride = 100;
float wcoX = 0, wcoY = 0, wcoZ = 0, wcoA = 0;

// WebSocket reporting
bool autoReportingEnabled = false;
unsigned long reportingSetupTime = 0;

// Debug control
bool debugWebSocket = false;  // Set to true only when debugging
// ===== END NEW GLOBAL VARIABLES =====

// WiFi AP mode flag
bool inAPMode = false;
bool webServerStarted = false;

// RTC availability flag
bool rtcAvailable = false;

// Timing
unsigned long lastTachRead = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastHistoryUpdate = 0;
unsigned long lastStatusRequest = 0;
unsigned long sessionStartTime = 0;
unsigned long buttonPressStart = 0;
bool buttonPressed = false;

// FluidNC connection state
bool fluidncConnectionAttempted = false;
unsigned long bootCompleteTime = 0;

// ========== Function Prototypes ==========
void setupWebServer();
String getMainHTML();
String getSettingsHTML();
String getAdminHTML();
String getWiFiConfigHTML();
String getConfigJSON();
String getStatusJSON();
// Display module functions are now in display/ui_modes.h and display/screen_renderer.h
// Sensor functions are now in sensors/sensors.h
// Network functions are now in network/network.h
// Utility functions are now in utils/utils.h

void IRAM_ATTR tachISR() {
  tachCounter++;
}

// ============ WEB SERVER HTML TEMPLATES (PROGMEM) ============
// Moved to individual html files in /data/web/
// HTML pages are now loaded from filesystem (see data/web/ directory)
// Storage manager loads from SD card (if available) or LittleFS fallback

// ============ WEB SERVER FUNCTIONS ============

void setup() {
  Serial.begin(115200);
  Serial.println("FluidDash - Starting...");

  // Initialize default configuration
  initDefaultConfig();

  // Enable watchdog timer (10 seconds)
  enableLoopWDT();
  Serial.println("Watchdog timer enabled (10s timeout)");

  // Initialize display (feed watchdog before long operation)
  yield();
  Serial.println("Initializing display...");
  gfx.init();
  gfx.setRotation(1);  // 90° rotation for landscape mode (480x320)
  gfx.setBrightness(255);
  Serial.println("Display initialized OK");
  gfx.fillScreen(COLOR_BG);
  showSplashScreen();

  // Show splash with non-blocking delay (feed watchdog)
  for (int i = 0; i < 20; i++) {
    delay(100);
    yield();
  }

  // Initialize hardware BEFORE drawing (RTC needed for datetime display)
  yield();
  Wire.begin(RTC_SDA, RTC_SCL);  // CYD I2C pins: GPIO32=SDA, GPIO25=SCL

  // Check if RTC is present
  if (!rtc.begin()) {
    Serial.println("RTC not found - time display will show 'No RTC'");
    rtcAvailable = false;
  } else {
    Serial.println("RTC initialized");
    rtcAvailable = true;
  }

  pinMode(BTN_MODE, INPUT_PULLUP);

  // Configure ADC & PWM
  analogSetWidth(12);
  analogSetAttenuation(ADC_11db);
  ledcSetup(0, PWM_FREQ, PWM_RESOLUTION);  // channel 0
  ledcAttachPin(FAN_PWM, 0);               // attach pin to channel 0
  ledcWrite(0, 0);
  pinMode(FAN_TACH, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FAN_TACH), tachISR, FALLING);

  // Initialize storage system (SD + LittleFS)
  Serial.println("Initializing storage...");
  if (!storage.begin()) {
    Serial.println("CRITICAL: Storage initialization failed!");
  }
  yield();

  // Load configuration (overwrites defaults with saved values)
  loadConfig();

  // Allocate history buffer based on config
  allocateHistoryBuffer();

  // Initialize DS18B20 temperature sensors
  yield();
  initDS18B20Sensors();

  // Load sensor configuration from NVS
  loadSensorConfig();

  // ========== WiFi Setup (Optional) ==========
  // Device works standalone without WiFi. WiFi enables web interface.

  // Read WiFi credentials from preferences
  prefs.begin("fluiddash", true);
  String wifi_ssid = prefs.getString("wifi_ssid", "");
  String wifi_pass = prefs.getString("wifi_pass", "");
  prefs.end();

  if (wifi_ssid.length() == 0) {
    // First boot or no credentials - enter AP mode for setup
    Serial.println("No WiFi credentials found - entering AP mode");
    Serial.println("Connect to 'FluidDash-Setup' WiFi to configure");

    WiFi.mode(WIFI_AP);
    WiFi.softAP("FluidDash-Setup");
    inAPMode = true;

    Serial.print("AP Mode - IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("[AP] Navigate to http://192.168.4.1/ to configure WiFi");

    // Start web server for configuration
    setupWebServer();
    webServerStarted = true;
    yield();
  } else {
    // Try to connect to saved WiFi
    Serial.println("WiFi credentials found");
    Serial.println("Connecting to: " + wifi_ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());

    yield();

    // Wait up to 5 seconds for connection (non-blocking with yields)
    int wifi_retry = 0;
    while (WiFi.status() != WL_CONNECTED && wifi_retry < 10) {
      delay(500);
      Serial.print(".");
      wifi_retry++;
      yield();
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      // Successfully connected to WiFi
      Serial.println("WiFi Connected!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());

      // Set up mDNS
      if (MDNS.begin(cfg.device_name)) {
        Serial.printf("mDNS started: http://%s.local\n", cfg.device_name);
        MDNS.addService("http", "tcp", 80);
      }

      yield();

      // Start web server
      setupWebServer();
      webServerStarted = true;
      yield();
    } else {
      // WiFi connection failed - continue in standalone mode
      Serial.println("WiFi connection failed");
      Serial.println("Device running standalone (temp/PSU/fan monitoring)");
      Serial.println("Hold button for 5+ seconds to enter WiFi setup mode");

      yield();
    }
  }

  yield();

  sessionStartTime = millis();
  currentMode = cfg.default_mode;

  yield();

  // Clear splash screen and draw the main interface
  Serial.println("Drawing main interface...");
  drawScreen();
  yield();

  // Mark boot complete time for deferred FluidNC connection
  bootCompleteTime = millis();
  Serial.println("Setup complete - entering main loop");
  yield();
}

void loop() {
  // NOTE: FluidNC connection is now only initiated via web interface
  // Device runs standalone by default for temperature/PSU monitoring

  // Handle web server requests
  server.handleClient();
  yield();
  handleButton();

  // Non-blocking ADC sampling (takes one sample every 5ms)
  sampleSensorsNonBlocking();

  // Process complete ADC readings when ready
  if (adcReady) {
    processAdcReadings();
    controlFan();
    adcReady = false;
  }

  if (millis() - lastTachRead >= 1000) {
    calculateRPM();
    lastTachRead = millis();
  }

  if (millis() - lastHistoryUpdate >= (cfg.graph_update_interval * 1000)) {
    updateTempHistory();
    lastHistoryUpdate = millis();
  }

  // WebSocket handling (only if connection was attempted)
  if (WiFi.status() == WL_CONNECTED && fluidncConnectionAttempted) {
      yield();  // Yield before WebSocket operations
      webSocket.loop();
      yield();  // Yield after WebSocket operations

      // Always poll for status - FluidNC doesn't have automatic reporting
      if (fluidncConnected && (millis() - lastStatusRequest >= cfg.status_update_rate)) {
          if (debugWebSocket) {
              Serial.println("[FluidNC] Sending status request");
          }
          yield();  // Yield before send
          webSocket.sendTXT("?");
          yield();  // Yield after send
          lastStatusRequest = millis();
      }

      // Periodic debug output (only every 10 seconds now)
      static unsigned long lastDebug = 0;
      if (debugWebSocket && millis() - lastDebug >= 10000) {
          Serial.printf("[DEBUG] State:%s MPos:(%.2f,%.2f,%.2f,%.2f) WPos:(%.2f,%.2f,%.2f,%.2f)\n",
                        machineState.c_str(),
                        posX, posY, posZ, posA,
                        wposX, wposY, wposZ, wposA);
          lastDebug = millis();
      }
  }


  if (millis() - lastDisplayUpdate >= 1000) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }

  // Short yield instead of delay for better responsiveness
  yield();
}

// ========== Configuration Management ==========

// allocateHistoryBuffer() is now in utils/utils.cpp

// ========== WiFiManager Setup ==========
// WiFi manager setup is now in network/network.cpp

// ========== Web Server Setup ==========

// Web server handler functions
void handleRoot() {
  server.send(200, "text/html", getMainHTML());
}

void handleSettings() {
  server.send(200, "text/html", getSettingsHTML());
}

void handleAdmin() {
  server.send(200, "text/html", getAdminHTML());
}

void handleWiFi() {
  server.send(200, "text/html", getWiFiConfigHTML());
}

void handleSensors() {
  // Load sensor configuration page from filesystem
  String html = storage.loadFile("/web/sensor_config.html");

  if (html.length() == 0) {
    Serial.println("[Web] ERROR: Failed to load sensor_config.html");
    html = "<html><body><h1>Error: sensor_config.html not found</h1></body></html>";
  }

  server.send(200, "text/html", html);
}

void handleDriverSetup() {
  // Load driver setup page from filesystem
  String html = storage.loadFile("/web/driver_setup.html");

  if (html.length() == 0) {
    Serial.println("[Web] ERROR: Failed to load driver_setup.html");
    html = "<html><body><h1>Error: driver_setup.html not found</h1></body></html>";
  }

  server.send(200, "text/html", html);
}

void handleAPIConfig() {
  server.send(200, "application/json", getConfigJSON());
}

void handleAPIStatus() {
  server.send(200, "application/json", getStatusJSON());
}

void handleAPISave() {
  // Update config from POST parameters
  if (server.hasArg("temp_low")) {
    cfg.temp_threshold_low = server.arg("temp_low").toFloat();
  }
  if (server.hasArg("temp_high")) {
    cfg.temp_threshold_high = server.arg("temp_high").toFloat();
  }
  if (server.hasArg("fan_min")) {
    cfg.fan_min_speed = server.arg("fan_min").toInt();
  }
  if (server.hasArg("graph_time")) {
    uint16_t newTime = server.arg("graph_time").toInt();
    if (newTime != cfg.graph_timespan_seconds) {
      cfg.graph_timespan_seconds = newTime;
      allocateHistoryBuffer(); // Reallocate with new size
    }
  }
  if (server.hasArg("graph_interval")) {
    cfg.graph_update_interval = server.arg("graph_interval").toInt();
  }
  if (server.hasArg("psu_low")) {
    cfg.psu_alert_low = server.arg("psu_low").toFloat();
  }
  if (server.hasArg("psu_high")) {
    cfg.psu_alert_high = server.arg("psu_high").toFloat();
  }
  if (server.hasArg("coord_decimals")) {
    cfg.coord_decimal_places = server.arg("coord_decimals").toInt();
  }

  saveConfig();
  server.send(200, "text/plain", "Settings saved successfully");
}

void handleAPIAdminSave() {
  if (server.hasArg("cal_x")) {
    cfg.temp_offset_x = server.arg("cal_x").toFloat();
  }
  if (server.hasArg("cal_yl")) {
    cfg.temp_offset_yl = server.arg("cal_yl").toFloat();
  }
  if (server.hasArg("cal_yr")) {
    cfg.temp_offset_yr = server.arg("cal_yr").toFloat();
  }
  if (server.hasArg("cal_z")) {
    cfg.temp_offset_z = server.arg("cal_z").toFloat();
  }
  if (server.hasArg("psu_cal")) {
    cfg.psu_voltage_cal = server.arg("psu_cal").toFloat();
  }

  saveConfig();
  server.send(200, "text/plain", "Calibration saved successfully");
}

void handleAPIResetWiFi() {
  wm.resetSettings();
  server.send(200, "text/plain", "WiFi settings cleared. Device will restart...");
  delay(1000);
  ESP.restart();
}

void handleAPIRestart() {
  server.send(200, "text/plain", "Restarting device...");
  delay(1000);
  ESP.restart();
}

void handleAPIWiFiConnect() {
  String ssid = "";
  String password = "";

  if (server.hasArg("ssid")) {
    ssid = server.arg("ssid");
  }
  if (server.hasArg("password")) {
    password = server.arg("password");
  }

  if (ssid.length() == 0) {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"SSID required\"}");
    return;
  }

  Serial.println("Attempting to connect to: " + ssid);

  // Store credentials in preferences
  prefs.begin("fluiddash", false);
  prefs.putString("wifi_ssid", ssid);
  prefs.putString("wifi_pass", password);
  prefs.end();

  // Send response and restart to apply credentials
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Credentials saved. Device will restart and attempt to connect.\"}");

  Serial.println("WiFi credentials saved. Restarting...");
  delay(2000);
  ESP.restart();
}

void handleAPIReloadScreens() {
    // PHASE 2 FINAL: Reboot-based workflow (avoids mutex/context issues)
    Serial.println("[API] Layout reload requested - rebooting device");

    server.send(200, "application/json",
        "{\"status\":\"Rebooting device to load new layouts...\",\"message\":\"Device will restart in 1 second\"}");

    delay(1000);  // Let response send
    ESP.restart();
}

void handleAPIRTC() {
    // Get current time from RTC
    DateTime now = rtc.now();

    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());

    String json = "{\"success\":true,\"timestamp\":\"";
    json += timestamp;
    json += "\"}";

    server.send(200, "application/json", json);
}

void handleAPIRTCSet() {
    // Parse date and time from POST parameters
    if (!server.hasArg("date") || !server.hasArg("time")) {
        server.send(400, "application/json",
            "{\"success\":false,\"error\":\"Missing date or time parameter\"}");
        return;
    }

    String dateStr = server.arg("date");  // Format: YYYY-MM-DD
    String timeStr = server.arg("time");  // Format: HH:MM:SS

    // Parse date: YYYY-MM-DD
    int year = dateStr.substring(0, 4).toInt();
    int month = dateStr.substring(5, 7).toInt();
    int day = dateStr.substring(8, 10).toInt();

    // Parse time: HH:MM:SS
    int hour = timeStr.substring(0, 2).toInt();
    int minute = timeStr.substring(3, 5).toInt();
    int second = timeStr.substring(6, 8).toInt();

    // Validate ranges
    if (year < 2000 || year > 2099 || month < 1 || month > 12 ||
        day < 1 || day > 31 || hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 || second < 0 || second > 59) {
        server.send(400, "application/json",
            "{\"success\":false,\"error\":\"Invalid date/time values\"}");
        return;
    }

    // Set the RTC time
    rtc.adjust(DateTime(year, month, day, hour, minute, second));

    Serial.printf("[RTC] Time set to: %04d-%02d-%02d %02d:%02d:%02d\n",
                  year, month, day, hour, minute, second);

    server.send(200, "application/json",
        "{\"success\":true,\"message\":\"RTC time updated successfully\"}");
}

// ========== Sensor Configuration API Handlers (Phase 7) ==========

// GET /api/sensors/discover - Scan OneWire bus for all DS18B20 sensors
// Returns: {"sensors": [{"uid": "28FF641E8C160450", "temp": 23.5}, ...]}
void handleAPISensorsDiscover() {
  JsonDocument doc;
  JsonArray sensors = doc["sensors"].to<JsonArray>();

  std::vector<String> uids = getDiscoveredUIDs();

  for (const String& uidStr : uids) {
    uint8_t uid[8];
    stringToUID(uidStr, uid);
    float temp = getTempByUID(uid);

    JsonObject sensor = sensors.add<JsonObject>();
    sensor["uid"] = uidStr;
    sensor["temp"] = isnan(temp) ? 0.0 : temp;
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// GET /api/sensors/list - Get configured sensor mappings
// Returns: {"sensors": [{"uid": "...", "name": "X-Driver", "alias": "temp0", "enabled": true, "notes": "..."}, ...]}
void handleAPISensorsList() {
  JsonDocument doc;
  JsonArray sensors = doc["sensors"].to<JsonArray>();

  for (const SensorMapping& mapping : sensorMappings) {
    JsonObject sensor = sensors.add<JsonObject>();
    sensor["uid"] = uidToString(mapping.uid);
    sensor["name"] = mapping.friendlyName;
    sensor["alias"] = mapping.alias;
    sensor["enabled"] = mapping.enabled;
    sensor["notes"] = mapping.notes;
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// POST /api/sensors/save - Save sensor mapping configuration
// Body: {"uid": "28FF641E8C160450", "name": "X-Driver", "alias": "temp0", "notes": "..."}
void handleAPISensorsSave() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing request body\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  // Parse request
  String uidStr = doc["uid"] | "";
  String name = doc["name"] | "";
  String alias = doc["alias"] | "";
  String notes = doc["notes"] | "";

  if (uidStr.length() != 16) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid UID format\"}");
    return;
  }

  if (name.length() == 0 || alias.length() == 0) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Name and alias are required\"}");
    return;
  }

  // Convert UID string to byte array
  uint8_t uid[8];
  stringToUID(uidStr, uid);

  // Add or update mapping
  bool success = addSensorMapping(uid, name.c_str(), alias.c_str());

  // Update notes if provided
  if (success && notes.length() > 0) {
    for (auto& mapping : sensorMappings) {
      if (memcmp(mapping.uid, uid, 8) == 0) {
        strlcpy(mapping.notes, notes.c_str(), sizeof(mapping.notes));
        saveSensorConfig();
        break;
      }
    }
  }

  if (success) {
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Sensor mapping saved\"}");
  } else {
    server.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save sensor mapping\"}");
  }
}

// GET /api/sensors/temps - Get real-time temperatures for all sensors
// Returns: {"sensors": [{"uid": "...", "name": "X-Driver", "alias": "temp0", "temp": 42.3}, ...]}
void handleAPISensorsTemps() {
  JsonDocument doc;
  JsonArray sensors = doc["sensors"].to<JsonArray>();

  // Return temps for configured sensors
  for (const SensorMapping& mapping : sensorMappings) {
    if (mapping.enabled) {
      float temp = getTempByUID(mapping.uid);

      JsonObject sensor = sensors.add<JsonObject>();
      sensor["uid"] = uidToString(mapping.uid);
      sensor["name"] = mapping.friendlyName;
      sensor["alias"] = mapping.alias;
      sensor["temp"] = isnan(temp) ? 0.0 : temp;
    }
  }

  // If no mappings, return temps for all discovered sensors
  if (sensorMappings.empty()) {
    std::vector<String> uids = getDiscoveredUIDs();
    for (size_t i = 0; i < uids.size(); i++) {
      uint8_t uid[8];
      stringToUID(uids[i], uid);
      float temp = getTempByUID(uid);

      JsonObject sensor = sensors.add<JsonObject>();
      sensor["uid"] = uids[i];
      sensor["name"] = "Sensor " + String(i);
      sensor["alias"] = "temp" + String(i);
      sensor["temp"] = isnan(temp) ? 0.0 : temp;
    }
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// POST /api/sensors/detect - Start touch detection to identify which sensor is being touched
// Body: {"timeout": 30000} (optional, default 30 seconds)
// Returns: {"uid": "28FF641E8C160450"} or {"uid": ""} on timeout
void handleAPISensorsDetect() {
  unsigned long timeout = 30000;  // Default 30 seconds

  if (server.hasArg("plain")) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (!error && doc.containsKey("timeout")) {
      timeout = doc["timeout"];
    }
  }

  Serial.println("[API] Starting touch detection...");
  String touchedUID = detectTouchedSensor(timeout, 1.0);

  JsonDocument doc;
  doc["uid"] = touchedUID;
  doc["success"] = (touchedUID.length() > 0);

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// ========== Driver Assignment API Handlers ==========

// GET /api/drivers/get - Get all driver position assignments
// Returns: {"drivers": [{"position": 0, "name": "X-Axis", "uid": "...", "temp": 23.5}, ...]}
void handleAPIDriversGet() {
  JsonDocument doc;
  JsonArray drivers = doc["drivers"].to<JsonArray>();

  const char* positionNames[] = {"X-Axis", "Y-Left", "Y-Right", "Z-Axis"};

  for (int pos = 0; pos < 4; pos++) {
    JsonObject driver = drivers.add<JsonObject>();
    driver["position"] = pos;
    driver["name"] = positionNames[pos];

    const SensorMapping* mapping = getSensorMappingByPosition(pos);
    if (mapping) {
      driver["uid"] = uidToString(mapping->uid);
      driver["assigned"] = true;
      float temp = getTempByUID(mapping->uid);
      driver["temp"] = isnan(temp) ? 0.0 : temp;
    } else {
      driver["uid"] = "";
      driver["assigned"] = false;
      driver["temp"] = 0.0;
    }
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// POST /api/drivers/assign - Assign sensor UID to driver position
// Body: {"position": 0, "uid": "28FF641E8C160450"}
void handleAPIDriversAssign() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing request body\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  int position = doc["position"] | -1;
  String uidStr = doc["uid"] | "";

  if (position < 0 || position > 3) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid position (must be 0-3)\"}");
    return;
  }

  if (uidStr.length() != 16) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid UID format\"}");
    return;
  }

  uint8_t uid[8];
  stringToUID(uidStr, uid);

  bool success = assignSensorToPosition(uid, position);

  if (success) {
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Driver assignment saved\"}");
  } else {
    server.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to assign sensor\"}");
  }
}

// POST /api/drivers/clear - Clear sensor assignment from position
// Body: {"position": 0}
void handleAPIDriversClear() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing request body\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  int position = doc["position"] | -1;

  if (position < 0 || position > 3) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid position (must be 0-3)\"}");
    return;
  }

  // Clear position assignment
  for (auto& mapping : sensorMappings) {
    if (mapping.displayPosition == position) {
      mapping.displayPosition = -1;
      saveSensorConfig();
      server.send(200, "application/json", "{\"success\":true,\"message\":\"Position cleared\"}");
      return;
    }
  }

  server.send(200, "application/json", "{\"success\":true,\"message\":\"Position was not assigned\"}");
}

void setupWebServer() {
  // Register all handlers
  server.on("/", HTTP_GET, handleRoot);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/admin", HTTP_GET, handleAdmin);
  server.on("/wifi", HTTP_GET, handleWiFi);
  server.on("/sensors", HTTP_GET, handleSensors);
  server.on("/driver_setup", HTTP_GET, handleDriverSetup);
  server.on("/api/config", HTTP_GET, handleAPIConfig);
  server.on("/api/status", HTTP_GET, handleAPIStatus);
  server.on("/api/save", HTTP_POST, handleAPISave);
  server.on("/api/admin/save", HTTP_POST, handleAPIAdminSave);
  server.on("/api/reset-wifi", HTTP_POST, handleAPIResetWiFi);
  server.on("/api/restart", HTTP_POST, handleAPIRestart);
  server.on("/api/reboot", HTTP_GET, []() {
      server.send(200, "application/json",
          "{\"status\":\"Rebooting device...\",\"message\":\"Device will restart in 1 second\"}");
      delay(1000);  // Let response send
      ESP.restart();
  });
  server.on("/api/wifi/connect", HTTP_POST, handleAPIWiFiConnect);
  server.on("/api/reload-screens", HTTP_POST, handleAPIReloadScreens);
  server.on("/api/reload-screens", HTTP_GET, handleAPIReloadScreens);  // Also accept GET
  server.on("/api/rtc", HTTP_GET, handleAPIRTC);
  server.on("/api/rtc/set", HTTP_POST, handleAPIRTCSet);

  // Sensor configuration API endpoints (Phase 7)
  server.on("/api/sensors/discover", HTTP_GET, handleAPISensorsDiscover);
  server.on("/api/sensors/list", HTTP_GET, handleAPISensorsList);
  server.on("/api/sensors/save", HTTP_POST, handleAPISensorsSave);
  server.on("/api/sensors/temps", HTTP_GET, handleAPISensorsTemps);
  server.on("/api/sensors/detect", HTTP_POST, handleAPISensorsDetect);

  // Driver assignment API endpoints
  server.on("/api/drivers/get", HTTP_GET, handleAPIDriversGet);
  server.on("/api/drivers/assign", HTTP_POST, handleAPIDriversAssign);
  server.on("/api/drivers/clear", HTTP_POST, handleAPIDriversClear);

  // REMOVED: JSON screen editor routes (Phase 2 - these handlers were never implemented)
  // server.on("/api/upload-status", HTTP_GET, handleUploadStatus);
  // server.on("/upload", HTTP_GET, handleUpload);
  // server.on("/upload-json", HTTP_POST, handleUploadComplete, handleUploadJSON);
  // server.on("/get-json", HTTP_GET, handleGetJSON);
  // server.on("/save-json", HTTP_POST, handleSaveJSON);
  // server.on("/editor", HTTP_GET, handleEditor);

  // 404 handler
  server.onNotFound([]() {
    server.send(404, "text/plain", "404: Page not found");
  });

  server.begin();
  Serial.println("Web server started");
}

// ========== HTML Pages ==========
String getMainHTML() {
  // Load from filesystem (SD or LittleFS)
  String html = storage.loadFile("/web/main.html");

  if (html.length() == 0) {
    Serial.println("[Web] ERROR: Failed to load main.html");
    return "<html><body><h1>Error: main.html not found</h1></body></html>";
  }

  // Replace all placeholders with dynamic content
  html.replace("%DEVICE_NAME%", cfg.device_name);
  html.replace("%IP_ADDRESS%", WiFi.localIP().toString());
  html.replace("%FLUIDNC_IP%", cfg.fluidnc_ip);

  return html;
}

String getSettingsHTML() {
  // Load from filesystem (SD or LittleFS)
  String html = storage.loadFile("/web/settings.html");

  if (html.length() == 0) {
    Serial.println("[Web] ERROR: Failed to load settings.html");
    return "<html><body><h1>Error: settings.html not found</h1></body></html>";
  }

  // Replace numeric input values
  html.replace("%TEMP_LOW%", String(cfg.temp_threshold_low));
  html.replace("%TEMP_HIGH%", String(cfg.temp_threshold_high));
  html.replace("%FAN_MIN%", String(cfg.fan_min_speed));
  html.replace("%PSU_LOW%", String(cfg.psu_alert_low));
  html.replace("%PSU_HIGH%", String(cfg.psu_alert_high));

  // Replace graph timespan selected options
  html.replace("%GRAPH_TIME_60%", cfg.graph_timespan_seconds == 60 ? "selected" : "");
  html.replace("%GRAPH_TIME_300%", cfg.graph_timespan_seconds == 300 ? "selected" : "");
  html.replace("%GRAPH_TIME_600%", cfg.graph_timespan_seconds == 600 ? "selected" : "");
  html.replace("%GRAPH_TIME_1800%", cfg.graph_timespan_seconds == 1800 ? "selected" : "");
  html.replace("%GRAPH_TIME_3600%", cfg.graph_timespan_seconds == 3600 ? "selected" : "");

  // Replace graph interval selected options
  html.replace("%GRAPH_INT_1%", cfg.graph_update_interval == 1 ? "selected" : "");
  html.replace("%GRAPH_INT_5%", cfg.graph_update_interval == 5 ? "selected" : "");
  html.replace("%GRAPH_INT_10%", cfg.graph_update_interval == 10 ? "selected" : "");
  html.replace("%GRAPH_INT_30%", cfg.graph_update_interval == 30 ? "selected" : "");
  html.replace("%GRAPH_INT_60%", cfg.graph_update_interval == 60 ? "selected" : "");

  // Replace coordinate decimal places selected options
  html.replace("%COORD_DEC_2%", cfg.coord_decimal_places == 2 ? "selected" : "");
  html.replace("%COORD_DEC_3%", cfg.coord_decimal_places == 3 ? "selected" : "");

  return html;
}

String getAdminHTML() {
  // Load from filesystem (SD or LittleFS)
  String html = storage.loadFile("/web/admin.html");

  if (html.length() == 0) {
    Serial.println("[Web] ERROR: Failed to load admin.html");
    return "<html><body><h1>Error: admin.html not found</h1></body></html>";
  }

  // Replace calibration offset values (with 2 decimal places for temp)
  html.replace("%CAL_X%", String(cfg.temp_offset_x, 2));
  html.replace("%CAL_YL%", String(cfg.temp_offset_yl, 2));
  html.replace("%CAL_YR%", String(cfg.temp_offset_yr, 2));
  html.replace("%CAL_Z%", String(cfg.temp_offset_z, 2));

  // Replace PSU calibration value (with 3 decimal places)
  html.replace("%PSU_CAL%", String(cfg.psu_voltage_cal, 3));

  return html;
}

String getWiFiConfigHTML() {
  // Load from filesystem (SD or LittleFS)
  String html = storage.loadFile("/web/wifi_config.html");

  if (html.length() == 0) {
    Serial.println("[Web] ERROR: Failed to load wifi_config.html");
    return "<html><body><h1>Error: wifi_config.html not found</h1></body></html>";
  }

  // Get current WiFi status
  String currentSSID = WiFi.SSID();
  String currentIP = WiFi.localIP().toString();
  bool isConnected = (WiFi.status() == WL_CONNECTED);
  bool isAPMode = inAPMode;

  // Build WiFi status section
  String wifiStatus = "<div class='status ";
  if (isAPMode) {
    wifiStatus += "status-ap'>🔧 AP Mode Active - Configure WiFi to connect to your network</div>";
  } else if (isConnected) {
    wifiStatus += "status-connected'>✅ Connected to: " + currentSSID + "<br>IP: " + currentIP + "</div>";
  } else {
    wifiStatus += "status-disconnected'>❌ Not Connected - Configure WiFi below</div>";
  }

  // Replace placeholders
  html.replace("%WIFI_STATUS%", wifiStatus);
  html.replace("%CURRENT_SSID%", currentSSID);

  return html;
}

// ========== JSON API Responses ==========
String getConfigJSON() {
  JsonDocument doc;

  // Network settings
  doc["device_name"] = cfg.device_name;
  doc["fluidnc_ip"] = cfg.fluidnc_ip;
  doc["fluidnc_port"] = cfg.fluidnc_port;
  doc["fluidnc_auto_discover"] = cfg.fluidnc_auto_discover;

  // Temperature settings
  doc["temp_threshold_low"] = cfg.temp_threshold_low;
  doc["temp_threshold_high"] = cfg.temp_threshold_high;
  doc["temp_offset_x"] = cfg.temp_offset_x;
  doc["temp_offset_yl"] = cfg.temp_offset_yl;
  doc["temp_offset_yr"] = cfg.temp_offset_yr;
  doc["temp_offset_z"] = cfg.temp_offset_z;

  // Fan settings
  doc["fan_min_speed"] = cfg.fan_min_speed;
  doc["fan_max_speed_limit"] = cfg.fan_max_speed_limit;

  // PSU settings
  doc["psu_voltage_cal"] = cfg.psu_voltage_cal;
  doc["psu_alert_low"] = cfg.psu_alert_low;
  doc["psu_alert_high"] = cfg.psu_alert_high;

  // Display settings
  doc["brightness"] = cfg.brightness;
  doc["default_mode"] = cfg.default_mode;
  doc["show_machine_coords"] = cfg.show_machine_coords;
  doc["show_temp_graph"] = cfg.show_temp_graph;
  doc["coord_decimal_places"] = cfg.coord_decimal_places;

  // Graph settings
  doc["graph_timespan_seconds"] = cfg.graph_timespan_seconds;
  doc["graph_update_interval"] = cfg.graph_update_interval;

  // Unit settings
  doc["use_fahrenheit"] = cfg.use_fahrenheit;
  doc["use_inches"] = cfg.use_inches;

  // System settings
  doc["enable_logging"] = cfg.enable_logging;
  doc["status_update_rate"] = cfg.status_update_rate;

  String output;
  serializeJson(doc, output);
  return output;
}

String getStatusJSON() {
  JsonDocument doc;

  // System status
  doc["uptime"] = millis() / 1000;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);

  // Temperatures
  JsonArray temps = doc["temperatures"].to<JsonArray>();
  for (int i = 0; i < 4; i++) {
    temps.add(temperatures[i]);
  }

  // PSU
  doc["psu_voltage"] = psuVoltage;
  doc["psu_min"] = psuMin;
  doc["psu_max"] = psuMax;

  // Fan
  doc["fan_rpm"] = fanRPM;
  doc["fan_speed"] = fanSpeed;

  // FluidNC status
  doc["fluidnc_connected"] = fluidncConnected;
  doc["machine_state"] = machineState;

  // Machine positions (work coordinates)
  JsonObject wpos = doc["wpos"].to<JsonObject>();
  wpos["x"] = wposX;
  wpos["y"] = wposY;
  wpos["z"] = wposZ;
  wpos["a"] = wposA;

  // Machine positions (machine coordinates)
  JsonObject mpos = doc["mpos"].to<JsonObject>();
  mpos["x"] = posX;
  mpos["y"] = posY;
  mpos["z"] = posZ;
  mpos["a"] = posA;

  // Work coordinate offsets
  JsonObject wco = doc["wco"].to<JsonObject>();
  wco["x"] = wcoX;
  wco["y"] = wcoY;
  wco["z"] = wcoZ;
  wco["a"] = wcoA;

  // Feed and spindle
  doc["feed_rate"] = feedRate;
  doc["spindle_rpm"] = spindleRPM;
  doc["feed_override"] = feedOverride;
  doc["rapid_override"] = rapidOverride;
  doc["spindle_override"] = spindleOverride;

  // Job status
  doc["is_job_running"] = isJobRunning;
  if (isJobRunning && jobStartTime > 0) {
    doc["job_duration"] = (millis() - jobStartTime) / 1000;
  } else {
    doc["job_duration"] = 0;
  }

  String output;
  serializeJson(doc, output);
  return output;
}

// ========== FluidNC Connection ==========
// FluidNC connection functions are now in network/network.cpp

// ========== Core Functions ==========
// Sensor functions are now in sensors/sensors.cpp
// Display functions are now in display/ui_modes.cpp

// ========== Watchdog Functions ==========
// (Watchdog function implementations would go here if needed)
