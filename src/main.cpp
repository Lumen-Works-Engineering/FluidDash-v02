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

#include "config/pins.h"
#include "config/config.h"
#include "display/display.h"
#include "display/ui_modes.h"
#include "sensors/sensors.h"
#include "network/network.h"
#include "utils/utils.h"
#include "web/web_utils.h"
#include "input/touch_handler.h"
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
#include "state/global_state.h"
#include "web/web_handlers.h"

void IRAM_ATTR tachISR() {
  sensors.tachCounter++;
}

void setup() {
  Serial.begin(115200);
  Serial.println("FluidDash - Starting...");
  
  // Initialize global state
  initGlobalState();
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
    network.rtcAvailable = false;
  } else {
    Serial.println("RTC initialized");
    network.rtcAvailable = true;
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
    network.inAPMode = true;

    Serial.print("AP Mode - IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("[AP] Navigate to http://192.168.4.1/ to configure WiFi");

    // Start web server for configuration
    setupWebServer();
    network.webServerStarted = true;
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
      network.webServerStarted = true;
      yield();

      // FluidNC connection (if enabled in settings)
      if (cfg.fluidnc_auto_discover) {
        Serial.println("[FluidNC] Auto-discover enabled - connecting...");
        connectFluidNC();
        fluidnc.connectionAttempted = true;
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

  timing.sessionStartTime = millis();
  currentMode = cfg.default_mode;

  yield();

  // Clear splash screen and draw the main interface
  Serial.println("Drawing main interface...");
  drawScreen();
  yield();

  // Mark boot complete time for deferred FluidNC connection
  timing.bootCompleteTime = millis();
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
  handleTouchInput();  // Handle touchscreen input

  // Non-blocking ADC sampling (takes one sample every 5ms)
  sampleSensorsNonBlocking();

  // Process complete ADC readings when ready
  if (sensors.adcReady) {
    processAdcReadings();
    controlFan();
    sensors.adcReady = false;
  }

  if (millis() - timing.lastTachRead >= 1000) {
    calculateRPM();
    timing.lastTachRead = millis();
  }

  if (millis() - timing.lastHistoryUpdate >= (cfg.graph_update_interval * 1000)) {
    updateTempHistory();
    timing.lastHistoryUpdate = millis();
  }

  // Handle WebSocket connection and status polling
  handleWebSocketLoop();


  if (millis() - timing.lastDisplayUpdate >= 1000) {
    updateDisplay();
    timing.lastDisplayUpdate = millis();
  }

  // Short yield instead of delay for better responsiveness
  yield();
}

