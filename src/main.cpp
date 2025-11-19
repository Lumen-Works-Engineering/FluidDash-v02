/*
 * FluidDash v0.2 - CYD Edition
 * Configured for ESP32-2432S028 (CYD 3.5" or 4.0" modules)
 *
 * Features:
 * - Standalone temperature/PSU monitoring (works without WiFi)
 * - 4× DS18B20 temperature sensors with touch-based position assignment
 * - PSU voltage monitoring and automatic fan control
 * - Optional WiFi (AP mode for setup, STA mode for operation)
 * - Web-based configuration interface
 * - Optional FluidNC CNC controller integration
 * - ETag-based HTTP caching for web performance
 * - NVS-based persistent configuration storage
 */

#include <Arduino.h>
#include "config/pins.h"
#include "config/config.h"
#include "display/display.h"
#include "display/ui_modes.h"
#include "sensors/sensors.h"
#include "network/network.h"
#include "utils/utils.h"
#include "web/web_utils.h"
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
#include <esp_task_wdt.h>
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

// ========== HTML & Web Resources ==========
// HTML pages load from filesystem (/data/web/) via StorageManager
// Dual-storage fallback: SD card (priority) → LittleFS
// ETag caching enabled for all HTML/JSON responses

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
      esp_task_wdt_reset();  // Feed watchdog during WiFi connection
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

      // FluidNC connection (if enabled in settings)
      if (cfg.fluidnc_auto_discover) {
        Serial.println("[FluidNC] Auto-discover enabled - connecting...");
        connectFluidNC();
        fluidncConnectionAttempted = true;
      }
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

