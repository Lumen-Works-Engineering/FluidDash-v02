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

// API handlers
void handleAPIConfig();
void handleAPIStatus();
void handleAPISave();
void handleAPIAdminSave();
// ... etc

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