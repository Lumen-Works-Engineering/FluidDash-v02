#ifndef WEB_UTILS_H
#define WEB_UTILS_H

#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <MD5Builder.h>

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

// ========== ETag Caching Support ==========

// Generate ETag from content using MD5 hash
// Returns ETag string in format: "md5hash"
// Usage: String etag = generateETag(htmlContent);
static String generateETag(const String& content) {
    MD5Builder md5;
    md5.begin();
    md5.add(content);
    md5.calculate();
    return "\"" + md5.toString() + "\"";  // ETags should be quoted
}

// Check if client's ETag matches current content
// Returns true if client has cached version (should send 304)
// Usage: if (checkETag(server, content)) return;
static bool checkETag(WebServer& server, const String& content) {
    // Generate ETag for current content
    String etag = generateETag(content);

    // Check if client sent If-None-Match header
    if (server.hasHeader("If-None-Match")) {
        String clientETag = server.header("If-None-Match");

        // If ETags match, client has current version
        if (clientETag == etag) {
            server.sendHeader("ETag", etag);
            server.send(304);  // 304 Not Modified
            return true;  // Client has cached version
        }
    }

    return false;  // Client needs updated content
}

// Send HTML response with ETag caching support
// Automatically checks If-None-Match and sends 304 if content matches
// Usage: sendHTMLWithETag(server, "text/html", htmlContent);
static void sendHTMLWithETag(WebServer& server, const char* contentType, const String& content) {
    // Check if client has cached version
    if (checkETag(server, content)) {
        return;  // 304 sent, done
    }

    // Generate ETag and send full content
    String etag = generateETag(content);
    server.sendHeader("ETag", etag);
    server.sendHeader("Cache-Control", "public, max-age=300");  // Cache for 5 minutes
    server.send(200, contentType, content);
}

#endif