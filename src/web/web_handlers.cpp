#include "web_handlers.h"
#include "state/global_state.h"
#include "display/display.h"
#include "config/config.h"
#include "sensors/sensors.h"
#include "network/network.h"
#include "utils/utils.h"
#include "web/web_utils.h"
#include "storage_manager.h"
#include "logging/data_logger.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <RTClib.h>
#include <ArduinoJson.h>
#include <Preferences.h>


// Web server handler functions
void handleRoot() {
  sendHTMLWithETag(server, "text/html", getMainHTML());
}

void handleSettings() {
  sendHTMLWithETag(server, "text/html", getSettingsHTML());
}

void handleAdmin() {
  sendHTMLWithETag(server, "text/html", getAdminHTML());
}

void handleWiFi() {
  sendHTMLWithETag(server, "text/html", getWiFiConfigHTML());
}

void handleSensors() {
  // Load sensor configuration page from filesystem
  String html = storage.loadFile("/web/sensor_config.html");

  if (html.length() == 0) {
    Serial.println("[Web] ERROR: Failed to load sensor_config.html");
    html = "<html><body><h1>Error: sensor_config.html not found</h1></body></html>";
  }

  sendHTMLWithETag(server, "text/html", html);
}

void handleDriverSetup() {
  // Load driver setup page from filesystem
  String html = storage.loadFile("/web/driver_setup.html");

  if (html.length() == 0) {
    Serial.println("[Web] ERROR: Failed to load driver_setup.html");
    html = "<html><body><h1>Error: driver_setup.html not found</h1></body></html>";
  }

  sendHTMLWithETag(server, "text/html", html);
}

void handleAPIConfig() {
  sendHTMLWithETag(server, "application/json", getConfigJSON());
}

void handleAPIStatus() {
  sendHTMLWithETag(server, "application/json", getStatusJSON());
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

  // FluidNC Integration Settings
  bool fluidncWasEnabled = cfg.fluidnc_auto_discover;
  bool fluidncNowEnabled = server.hasArg("fluidnc_enabled");
  cfg.fluidnc_auto_discover = fluidncNowEnabled;

  if (server.hasArg("fluidnc_ip")) {
    String ip = server.arg("fluidnc_ip");
    strlcpy(cfg.fluidnc_ip, ip.c_str(), sizeof(cfg.fluidnc_ip));
  }

  if (server.hasArg("fluidnc_port")) {
    cfg.fluidnc_port = server.arg("fluidnc_port").toInt();
  }

  saveConfig();

  // If FluidNC was just enabled, connect immediately
  if (!fluidncWasEnabled && fluidncNowEnabled && WiFi.status() == WL_CONNECTED) {
    Serial.println("[FluidNC] Enabled via settings - connecting...");
    connectFluidNC();
    fluidnc.connectionAttempted = true;
  }
  // If FluidNC was disabled, disconnect
  else if (fluidncWasEnabled && !fluidncNowEnabled) {
    Serial.println("[FluidNC] Disabled via settings - disconnecting...");
    webSocket.disconnect();
    fluidnc.connectionAttempted = false;
    fluidnc.connected = false;
    fluidnc.machineState = "OFFLINE";
  }

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

// handleAPIReloadScreens() removed - JSON screen rendering disabled in Phase 2

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
        sendJsonError(server, 400, "Missing required parameters", "Both 'date' and 'time' are required");
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
        sendJsonError(server, 400, "Invalid date/time values",
                     "Date must be YYYY-MM-DD, time must be HH:MM:SS");
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
    sendJsonError(server, 400, "Missing request body", "POST body with JSON required");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    sendJsonError(server, 400, "Invalid JSON", error.c_str());
    return;
  }

  // Parse request
  String uidStr = doc["uid"] | "";
  String name = doc["name"] | "";
  String alias = doc["alias"] | "";
  String notes = doc["notes"] | "";

  if (uidStr.length() != 16) {
    sendJsonError(server, 400, "Invalid UID format", "UID must be 16 hex characters (e.g., 28FF641E8C160450)");
    return;
  }

  if (name.length() == 0 || alias.length() == 0) {
    sendJsonError(server, 400, "Missing required fields", "Both 'name' and 'alias' are required");
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
    if (!error && !doc["timeout"].isNull()) {
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
    sendJsonError(server, 400, "Missing request body", "POST body with JSON required");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    sendJsonError(server, 400, "Invalid JSON", error.c_str());
    return;
  }

  int position = doc["position"] | -1;
  String uidStr = doc["uid"] | "";

  if (position < 0 || position > 3) {
    sendJsonError(server, 400, "Invalid position", "Position must be 0-3 (0=X, 1=YL, 2=YR, 3=Z)");
    return;
  }

  if (uidStr.length() != 16) {
    sendJsonError(server, 400, "Invalid UID format", "UID must be 16 hex characters");
    return;
  }

  uint8_t uid[8];
  stringToUID(uidStr, uid);

  bool success = assignSensorToPosition(uid, position);

  if (success) {
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Driver assignment saved\"}");
  } else {
    sendJsonError(server, 500, "Failed to assign sensor", "Check serial output for details");
  }
}

// POST /api/drivers/clear - Clear sensor assignment from position
// Body: {"position": 0}
void handleAPIDriversClear() {
  if (!server.hasArg("plain")) {
    sendJsonError(server, 400, "Missing request body", "POST body with JSON required");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    sendJsonError(server, 400, "Invalid JSON", error.c_str());
    return;
  }

  int position = doc["position"] | -1;

  if (position < 0 || position > 3) {
    sendJsonError(server, 400, "Invalid position", "Position must be 0-3 (0=X, 1=YL, 2=YR, 3=Z)");
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

// ========== Data Logger API Handlers (Phase 3) ==========

// POST /api/logs/enable - Enable or disable data logging
// Body: {"enabled": true, "interval": 10000} (interval in milliseconds, optional)
void handleAPILogsEnable() {
  if (!server.hasArg("plain")) {
    sendJsonError(server, 400, "Missing request body", "POST body with JSON required");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    sendJsonError(server, 400, "Invalid JSON", error.c_str());
    return;
  }

  bool enabled = doc["enabled"] | false;
  logger.setEnabled(enabled);

  if (doc.containsKey("interval")) {
    unsigned long interval = doc["interval"];
    if (interval >= 1000 && interval <= 600000) {  // 1s to 10min
      logger.setInterval(interval);
    }
  }

  JsonDocument response;
  response["success"] = true;
  response["enabled"] = logger.isEnabled();
  response["message"] = enabled ? "Logging enabled" : "Logging disabled";

  String output;
  serializeJson(response, output);
  server.send(200, "application/json", output);
}

// GET /api/logs/status - Get current logging status
void handleAPILogsStatus() {
  JsonDocument doc;
  doc["enabled"] = logger.isEnabled();
  doc["currentFile"] = logger.getCurrentLogFilename();

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

// GET /api/logs/list - List all log files
// Returns: {"files": ["2025-01-20_143022.csv", "2025-01-20_150015.csv"], "count": 2}
void handleAPILogsList() {
  std::vector<String> files = logger.listLogFiles();

  JsonDocument doc;
  JsonArray filesArray = doc["files"].to<JsonArray>();

  for (const String& file : files) {
    filesArray.add(file);
  }

  doc["count"] = files.size();

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

// GET /api/logs/download?file=<filename> - Download a specific log file
void handleAPILogsDownload() {
  if (!server.hasArg("file")) {
    sendJsonError(server, 400, "Missing parameter", "'file' parameter required");
    return;
  }

  String filename = server.arg("file");

  // Security: prevent directory traversal
  if (filename.indexOf("..") >= 0 || filename.indexOf("/") >= 0) {
    sendJsonError(server, 400, "Invalid filename", "Filename cannot contain '..' or '/'");
    return;
  }

  String filepath = String("/logs/") + filename;

  if (!storage.exists(filepath.c_str())) {
    sendJsonError(server, 404, "File not found", "Log file does not exist");
    return;
  }

  File file = storage.openFile(filepath.c_str(), FILE_READ);
  if (!file) {
    sendJsonError(server, 500, "Failed to open file", "Could not open log file");
    return;
  }

  // Stream the file to client
  server.sendHeader("Content-Disposition", "attachment; filename=" + filename);
  server.streamFile(file, "text/csv");
  file.close();
}

// DELETE /api/logs/clear - Delete all log files
void handleAPILogsClear() {
  bool success = logger.deleteAllLogs();

  JsonDocument doc;
  doc["success"] = success;
  doc["message"] = success ? "All log files deleted" : "Failed to delete log files";

  String output;
  serializeJson(doc, output);
  server.send(success ? 200 : 500, "application/json", output);
}

// ========== Web Server Setup ==========

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
  // /api/reload-screens removed - JSON screen rendering disabled in Phase 2
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

  // Data logger API endpoints (Phase 3)
  server.on("/api/logs/enable", HTTP_POST, handleAPILogsEnable);
  server.on("/api/logs/status", HTTP_GET, handleAPILogsStatus);
  server.on("/api/logs/list", HTTP_GET, handleAPILogsList);
  server.on("/api/logs/download", HTTP_GET, handleAPILogsDownload);
  server.on("/api/logs/clear", HTTP_DELETE, handleAPILogsClear);

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

  // Replace FluidNC integration settings
  html.replace("%FLUIDNC_ENABLED%", cfg.fluidnc_auto_discover ? "checked" : "");
  html.replace("%FLUIDNC_IP%", String(cfg.fluidnc_ip));
  html.replace("%FLUIDNC_PORT%", String(cfg.fluidnc_port));

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
  bool isAPMode = network.inAPMode;

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
    temps.add(sensors.temperatures[i]);
  }

  // PSU
  doc["psu_voltage"] = sensors.psuVoltage;
  doc["psu_min"] = sensors.psuMin;
  doc["psu_max"] = sensors.psuMax;

  // Fan
  doc["fan_rpm"] = sensors.fanRPM;
  doc["fan_speed"] = sensors.fanSpeed;

  // FluidNC status
  doc["fluidnc_connected"] = fluidnc.connected;
  doc["machine_state"] = fluidnc.machineState;

  // Machine positions (work coordinates)
  JsonObject wpos = doc["wpos"].to<JsonObject>();
  wpos["x"] = fluidnc.wposX;
  wpos["y"] = fluidnc.wposY;
  wpos["z"] = fluidnc.wposZ;
  wpos["a"] = fluidnc.wposA;

  // Machine positions (machine coordinates)
  JsonObject mpos = doc["mpos"].to<JsonObject>();
  mpos["x"] = fluidnc.posX;
  mpos["y"] = fluidnc.posY;
  mpos["z"] = fluidnc.posZ;
  mpos["a"] = fluidnc.posA;

  // Work coordinate offsets
  JsonObject wco = doc["wco"].to<JsonObject>();
  wco["x"] = fluidnc.wcoX;
  wco["y"] = fluidnc.wcoY;
  wco["z"] = fluidnc.wcoZ;
  wco["a"] = fluidnc.wcoA;

  // Feed and spindle
  doc["feed_rate"] = fluidnc.feedRate;
  doc["spindle_rpm"] = fluidnc.spindleRPM;
  doc["feed_override"] = fluidnc.feedOverride;
  doc["rapid_override"] = fluidnc.rapidOverride;
  doc["spindle_override"] = fluidnc.spindleOverride;

  // Job status
  doc["is_job_running"] = fluidnc.isJobRunning;
  if (fluidnc.isJobRunning && fluidnc.jobStartTime > 0) {
    doc["job_duration"] = (millis() - fluidnc.jobStartTime) / 1000;
  } else {
    doc["job_duration"] = 0;
  }

  String output;
  serializeJson(doc, output);
  return output;
}
