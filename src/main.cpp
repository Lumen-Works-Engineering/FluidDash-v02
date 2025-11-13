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
// const char MAIN_HTML[] PROGMEM = R"rawliteral()rawliteral";
// const char SETTINGS_HTML[] PROGMEM = R"rawliteral()rawliteral";
// const char ADMIN_HTML[] PROGMEM = R"rawliteral()rawliteral";
// const char WIFI_CONFIG_HTML[] PROGMEM = R"rawliteral()rawliteral";

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
  delay(2000);  // Show splash briefly

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

  // Load configuration (overwrites defaults with saved values)
  loadConfig();

  // Allocate history buffer based on config
  allocateHistoryBuffer();

  // Initialize DS18B20 temperature sensors
  yield();
  initDS18B20Sensors();

  // Try to connect to saved WiFi credentials
  Serial.println("Attempting WiFi connection...");

  // Read WiFi credentials from preferences
  prefs.begin("fluiddash", true);
  String wifi_ssid = prefs.getString("wifi_ssid", "");
  String wifi_pass = prefs.getString("wifi_pass", "");
  prefs.end();

  if (wifi_ssid.length() > 0) {
    Serial.println("Connecting to: " + wifi_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
  } else {
    Serial.println("No saved WiFi credentials");
    WiFi.mode(WIFI_STA);
  }

  yield();

  // Wait up to 10 seconds for connection
  int wifi_retry = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_retry < 20) {
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

    // Connect to FluidNC
    if (cfg.fluidnc_auto_discover) {
      discoverFluidNC();
    } else {
      connectFluidNC();
    }
    yield();
  } else {
    // WiFi connection failed - continue in standalone mode
    Serial.println("WiFi connection failed - continuing in standalone mode");
    Serial.println("Hold button for 10 seconds to enter WiFi configuration mode");

    yield();
  }

  yield();

  // Start web server (always available in STA, AP, or standalone mode)
  Serial.println("Starting web server...");
  setupWebServer();
  yield();

  sessionStartTime = millis();
  currentMode = cfg.default_mode;

  yield();
  delay(2000);
  yield();

  // Clear splash screen and draw the main interface
  Serial.println("Drawing main interface...");
  drawScreen();
  yield();

  Serial.println("Setup complete - entering main loop");
  yield();
}

void loop() {
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

  if (WiFi.status() == WL_CONNECTED) {
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

void setupWebServer() {
  // Register all handlers
  server.on("/", HTTP_GET, handleRoot);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/admin", HTTP_GET, handleAdmin);
  server.on("/wifi", HTTP_GET, handleWiFi);
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
  // PHASE 2: Re-enabled with SPIFFS-based upload (safe)
  server.on("/api/upload-status", HTTP_GET, handleUploadStatus);
  server.on("/upload", HTTP_GET, handleUpload);
  server.on("/upload-json", HTTP_POST, handleUploadComplete, handleUploadJSON);
  server.on("/get-json", HTTP_GET, handleGetJSON);
  server.on("/save-json", HTTP_POST, handleSaveJSON);
  server.on("/editor", HTTP_GET, handleEditor);

  server.begin();
  Serial.println("Web server started");
}

// ========== HTML Pages ==========
String getMainHTML() {
  String html = String(FPSTR(MAIN_HTML));

  // Replace all placeholders with dynamic content
  html.replace("%DEVICE_NAME%", cfg.device_name);
  html.replace("%IP_ADDRESS%", WiFi.localIP().toString());
  html.replace("%FLUIDNC_IP%", cfg.fluidnc_ip);

  return html;
}

String getSettingsHTML() {
  String html = String(FPSTR(SETTINGS_HTML));

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
  String html = String(FPSTR(ADMIN_HTML));

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
  String html = String(FPSTR(WIFI_CONFIG_HTML));

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

// ========== FluidNC Connection ==========
// FluidNC connection functions are now in network/network.cpp

// ========== Core Functions ==========
// Sensor functions are now in sensors/sensors.cpp
// Display functions are now in display/ui_modes.cpp

// ========== Watchdog Functions ==========
// (Watchdog function implementations would go here if needed)
