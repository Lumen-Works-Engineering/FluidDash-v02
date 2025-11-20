# Phase 4: Web Server Optimization - Implementation Plan

**Date:** 2025-01-18
**Status:** READY TO START
**Previous Phase:** Phase 7 & Driver Assignment (COMPLETE ✅)

---

## Overview

Phase 4 focuses on optimizing the web server for better performance and user experience, particularly for AP mode setup and file caching.

**Priorities:**
- 🔥 **HIGH:** ETag caching (performance)
- 🔥 **HIGH:** Captive portal for AP mode (UX)
- 📊 **MEDIUM:** JSON error responses (debugging)
- 📉 **LOW:** WebSocket keep-alive pings (optional FluidNC only)

---

## Task 1: ETag Caching for Static Files 🔥 HIGH PRIORITY

**Goal:** Reduce bandwidth usage by caching HTML/CSS/JS files in browser

**Benefits:**
- Faster page loads (304 Not Modified responses)
- Reduced bandwidth usage (~80% reduction for repeat visits)
- Better mobile experience

**Implementation Strategy:**

### 1.1: Add ETag Header to File Responses

**File:** `src/main.cpp` (web server handlers)

**Current behavior:**
```cpp
server.send(200, "text/html", htmlContent);
```

**New behavior:**
```cpp
String etag = "\"" + String(fileSize) + "-" + String(lastModified) + "\"";
if (server.hasHeader("If-None-Match") && server.header("If-None-Match") == etag) {
  server.send(304);  // Not Modified
} else {
  server.sendHeader("ETag", etag);
  server.sendHeader("Cache-Control", "max-age=86400");  // 24 hours
  server.send(200, "text/html", htmlContent);
}
```

**Locations to modify:**
- `/` handler (main.html)
- `/settings` handler
- `/admin` handler
- `/sensors` handler
- `/driver_setup` handler

**Testing:**
- First request: Should return 200 with ETag header
- Second request with If-None-Match: Should return 304
- After firmware update: Should return 200 with new ETag

---

## Task 2: Captive Portal for AP Mode 🔥 HIGH PRIORITY

**Goal:** Auto-redirect users to configuration page when connected to AP

**Benefits:**
- Better first-time setup UX
- No need to manually type IP address
- Standard behavior users expect from WiFi setup

**Implementation Strategy:**

### 2.1: Add DNS Server for Captive Portal

**File:** `src/main.cpp`

**Add includes:**
```cpp
#include <DNSServer.h>
```

**Add global variables:**
```cpp
DNSServer dnsServer;
const byte DNS_PORT = 53;
```

### 2.2: Start DNS Server in AP Mode

**File:** `src/main.cpp` (setup function, AP mode section)

```cpp
if (inAPMode) {
  // Start DNS server for captive portal
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  Serial.println("[AP] DNS server started for captive portal");
}
```

### 2.3: Process DNS Requests in Loop

**File:** `src/main.cpp` (loop function)

```cpp
if (inAPMode) {
  dnsServer.processNextRequest();
}
```

### 2.4: Add Captive Portal Detection Handlers

**File:** `src/main.cpp` (web server handlers)

```cpp
// Handle captive portal detection requests
server.on("/generate_204", HTTP_GET, []() {
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
});

server.on("/fwlink", HTTP_GET, []() {
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
});

// Catch-all handler for captive portal
server.onNotFound([]() {
  if (inAPMode) {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  } else {
    server.send(404, "text/plain", "Not found");
  }
});
```

**Testing:**
- Connect to "FluidDash-Setup" AP
- Mobile/laptop should auto-open browser to config page
- Typing any URL should redirect to 192.168.4.1

---

## Task 3: Improved JSON Error Responses 📊 MEDIUM PRIORITY

**Goal:** Standardize error responses for better debugging

**Implementation Strategy:**

### 3.1: Add JSON Error Helper Function

**File:** `src/main.cpp`

```cpp
// Send standardized JSON error response
void sendJsonError(int statusCode, const char* errorMessage, const char* details = nullptr) {
  JsonDocument doc;
  doc["success"] = false;
  doc["error"] = errorMessage;
  if (details) {
    doc["details"] = details;
  }
  doc["timestamp"] = millis();

  String response;
  serializeJson(doc, response);
  server.send(statusCode, "application/json", response);
}
```

### 3.2: Update API Handlers to Use JSON Errors

**Replace current error responses:**

**Before:**
```cpp
server.send(400, "text/plain", "Invalid request");
```

**After:**
```cpp
sendJsonError(400, "Invalid request", "Missing required field: uid");
```

**Locations to update:**
- `/api/sensors/save`
- `/api/sensors/detect`
- `/api/drivers/assign`
- `/api/drivers/clear`
- `/api/config` (POST)

**Testing:**
- Send invalid API requests
- Verify JSON error responses with proper structure
- Check that details field provides useful debugging info

---

## Task 4: WebSocket Keep-Alive Pings 📉 LOW PRIORITY (OPTIONAL)

**Goal:** Keep FluidNC WebSocket connection alive during idle periods

**Note:** Only relevant if FluidNC integration is enabled by user.

**Implementation Strategy:**

### 4.1: Add Ping Timer

**File:** `src/network/network.cpp`

```cpp
unsigned long lastWsPing = 0;
const unsigned long WS_PING_INTERVAL = 10000;  // 10 seconds
```

### 4.2: Send Periodic Pings

**File:** `src/main.cpp` (WebSocket loop section)

```cpp
if (fluidncConnected && (millis() - lastWsPing >= WS_PING_INTERVAL)) {
  webSocket.sendPing();
  lastWsPing = millis();
}
```

**Testing:**
- Enable FluidNC connection
- Monitor WebSocket for ping frames every 10 seconds
- Verify connection stays alive during idle periods

---

## Files to Modify Summary

| File | Task | Lines Affected |
|------|------|----------------|
| `src/main.cpp` | ETag caching | ~50-100 (web handlers) |
| `src/main.cpp` | Captive portal | ~30-40 (DNS + handlers) |
| `src/main.cpp` | JSON errors | ~20-30 (helper function + updates) |
| `src/network/network.cpp` | WebSocket pings | ~10-15 (optional) |
| `platformio.ini` | Add DNSServer library | 1 line |

---

## Testing Checklist

### ETag Caching
- [ ] First request returns 200 with ETag header
- [ ] Second request with If-None-Match returns 304
- [ ] Browser doesn't re-download cached files
- [ ] File changes trigger new ETag

### Captive Portal
- [ ] Connect to FluidDash-Setup AP
- [ ] Mobile browser auto-opens config page
- [ ] Typing any URL redirects to 192.168.4.1
- [ ] Normal STA mode not affected

### JSON Error Responses
- [ ] Invalid API requests return structured JSON
- [ ] Error messages are clear and actionable
- [ ] Details field provides debugging context
- [ ] Status codes are appropriate (400, 404, 500)

### WebSocket Pings (Optional)
- [ ] Pings sent every 10 seconds when FluidNC connected
- [ ] Connection stays alive during idle periods
- [ ] No impact when FluidNC not connected

---

## Implementation Order

**Recommended sequence:**

1. **Task 2: Captive Portal** (Biggest UX impact, relatively simple)
2. **Task 3: JSON Error Responses** (Foundation for better debugging)
3. **Task 1: ETag Caching** (Performance boost, more complex)
4. **Task 4: WebSocket Pings** (Optional, only if user needs FluidNC)

**Estimated Time:**
- Task 2: 1 hour
- Task 3: 30 minutes
- Task 1: 1-2 hours
- Task 4: 15 minutes
- **Total: 3-4 hours**

---

## Success Criteria

**Phase 4 Complete When:**
- ✅ Captive portal auto-opens config page in AP mode
- ✅ Static files return 304 Not Modified on repeat requests
- ✅ API errors return structured JSON responses
- ✅ (Optional) WebSocket pings keep FluidNC connection alive

**Ready to proceed?** Let's start with Task 2 (Captive Portal) for the biggest UX improvement! 🚀
