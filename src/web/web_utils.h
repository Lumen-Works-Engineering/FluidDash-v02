#ifndef WEB_UTILS_H
#define WEB_UTILS_H

#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// ========== LittleFS Utilities ==========

static bool initLittleFS(bool formatOnFail = true) {
    if (!LittleFS.begin(formatOnFail)) {
        return false;
    }
    return true;
}

static String loadFile(const char* path) {
    File file = LittleFS.open(path, "r");
    String contents = file.readString();
    file.close();
    return contents;
}

// ========== JSON Error Response Helper ==========

// Send standardized JSON error response with proper HTTP status code
// Usage: sendJsonError(server, 400, "Invalid request", "Missing required field: uid")
static void sendJsonError(WebServer& server, int statusCode, const char* errorMessage, const char* details = nullptr) {
    JsonDocument doc;
    doc["success"] = false;
    doc["error"] = errorMessage;
    if (details != nullptr && strlen(details) > 0) {
        doc["details"] = details;
    }
    doc["timestamp"] = millis();

    String response;
    serializeJson(doc, response);
    server.send(statusCode, "application/json", response);

    Serial.printf("[API] Error %d: %s%s%s\n",
                  statusCode,
                  errorMessage,
                  details ? " - " : "",
                  details ? details : "");
}

#endif