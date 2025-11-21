// Create: src/web/web_handlers.h
#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <WebServer.h>

// Web page handlers
void handleRoot();
void handleSettings();
void handleAdmin();
void handleWiFi();
void handleSensors();
void handleDriverSetup();
void handleLogs();

// API handlers
void handleAPIConfig();
void handleAPIStatus();
void handleAPISave();
void handleAPIAdminSave();
void handleAPIResetWiFi();
void handleAPIRestart();
void handleAPIResetToDefaults();
void handleAPIWiFiConnect();
void handleAPIRTC();
void handleAPIRTCSet();
void handleAPISensorsDiscover();
void handleAPISensorsList();
void handleAPISensorsSave();
void handleAPISensorsTemps();
void handleAPISensorsDetect();
void handleAPIDriversGet();
void handleAPIDriversAssign();
void handleAPIDriversClear();
// Data logger API handlers (Phase 3)
void handleAPILogsEnable();
void handleAPILogsStatus();
void handleAPILogsList();
void handleAPILogsDownload();
void handleAPILogsClear();

// HTML generators
String getMainHTML();
String getSettingsHTML();
String getAdminHTML();
String getWiFiConfigHTML();
String getConfigJSON();
String getStatusJSON();

// Web server setup
void setupWebServer();

#endif