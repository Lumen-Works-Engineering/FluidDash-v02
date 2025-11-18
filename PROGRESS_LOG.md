# FluidDash v02 - Development Progress Log

**Project Start:** 2025-01-17
**Last Updated:** 2025-01-18

---

## Session 1: 2025-01-17

### Completed Work:
- ✅ Phase 1: Storage System & HTML Integration
- ✅ Phase 2: JSON Screen Rendering Disabled
- ✅ Critical Fix: Watchdog boot loop resolved
- ✅ Architecture Refactor: WiFi made optional

### Key Changes:
- Storage properly initialized via `storage.begin()`
- HTML files load from filesystem (SD/LittleFS fallback)
- Hard-coded screen rendering exclusively used
- WiFi connection timeout reduced to 5 seconds
- Boot time: ~5-8 seconds (no watchdog timeouts)

---

## Session 2: 2025-01-18

### Phase 7: Temperature Sensor Naming & Driver Assignment ✅ COMPLETE

**Completed Features:**
1. **Driver Assignment System**
   - Automated touch detection for sensor-to-position mapping
   - Web UI at `/driver_setup` page
   - NVS persistence for assignments
   - Position-based display (0=X, 1=YL, 2=YR, 3=Z)

2. **Touch Detection Algorithm**
   - Temperature rise detection (1°C threshold)
   - 30-second timeout with graceful handling
   - Auto-naming sensors based on position

**Critical Bugs Fixed:**

### Bug #1: Sensor Position Mapping ✅ FIXED
- **Location:** `src/sensors/sensors.cpp:129-141`
- **Problem:** Temperatures stored using array index instead of displayPosition
- **Impact:** Sensors appeared in wrong LCD positions
- **Fix:** Changed `temperatures[i]` to `temperatures[pos]` where `pos = displayPosition`
- **Also Fixed:** Peak temperature tracking in `src/display/ui_modes.cpp:178-212`

### Bug #2: Watchdog Timeout During Touch Detection ✅ FIXED (Iteration 2)
- **Location:** `src/sensors/sensors.cpp:439-500`
- **Problem:** 30-second detection blocked main loop, exceeded 10s watchdog
- **Error:** `E (28936) task_wdt: Task watchdog got triggered`
- **First Attempt:** Added `yield()` - delayed timeout to 29s but still crashed
- **Final Solution:** Added `#include <esp_task_wdt.h>` and explicit `esp_task_wdt_reset()` calls every 50ms
- **Locations:** 3 strategic points in detection loop (lines 455, 478, 495)
- **Result:** Full 30-second detection runs without timeout

### Bug #3: Watchdog Timeout During WiFi Connection ✅ FIXED
- **Location:** `src/main.cpp:247-253`
- **Problem:** WiFi connection took >10s on first boot, causing reboot loop
- **Fix:** Added `esp_task_wdt_reset()` to WiFi connection loop
- **Result:** Clean boot without timeout/reboot cycle

---

## Session 2 (Continued): Phase 4 - Web Server Optimization

### Task 1: Captive Portal ❌ REMOVED (Not Needed)
- Attempted implementation with DNSServer
- User feedback: UDP errors, doesn't work reliably
- Decision: Manual AP setup (192.168.4.1) is sufficient
- **Status:** Reverted all captive portal code

### Task 2: JSON Error Responses ✅ COMPLETE
**Completed Features:**
1. **Created `sendJsonError()` Helper Function**
   - **Location:** `src/web/web_utils.h:29-47`
   - Standardized JSON format: `{success, error, details, timestamp}`
   - HTTP status code support (400, 500, etc.)
   - Serial logging for debugging

2. **Updated API Handlers:**
   - `handleAPIRTCSet()` - 2 error cases
   - `handleAPISensorsSave()` - 4 error cases
   - `handleAPIDriversAssign()` - 4 error cases
   - `handleAPIDriversClear()` - 3 error cases

3. **Code Organization:**
   - Moved helper from main.cpp to web_utils.h (keeping main.cpp lean)
   - Added ArduinoJson include to web_utils.h

**Example Error Response:**
```json
{
  "success": false,
  "error": "Invalid position",
  "details": "Position must be 0-3 (0=X, 1=YL, 2=YR, 3=Z)",
  "timestamp": 123456
}
```

### FluidNC Configuration UI ✅ COMPLETE (Unplanned Essential Feature)

**Completed Features:**
1. **Settings Page UI** (`data/web/settings.html`)
   - Added FluidNC Integration card
   - Enable/Disable checkbox
   - IP address or hostname input (supports mDNS like "fluidnc.local")
   - Port configuration (default: 81)
   - Helpful tooltips and info text
   - Clear messaging that feature is optional

2. **Backend Implementation** (`src/main.cpp`)
   - **Config Saving:** Lines 465-494 in `handleAPISave()`
   - **Template Rendering:** Lines 991-994 in `getSettingsHTML()`
   - **Auto-connect on Boot:** Lines 275-280 in `setup()`
   - **Immediate Connect/Disconnect:** When settings change (no reboot needed!)

3. **Connection Logic:**
   - If enabled: Auto-connects to FluidNC on boot
   - Settings change: Immediate connect/disconnect
   - WebSocket loop only runs when connection attempted (safe default)

**Testing Results:**
- ✅ FluidNC connects successfully via mDNS hostname
- ✅ Connection persists across reboots
- ✅ Enable/disable works without reboot
- ✅ Clean boot logs, no watchdog timeouts

**Commit Message Used:**
```
feat: Add FluidNC configuration UI and fix watchdog timeouts

- Add FluidNC integration card to settings.html with enable/disable checkbox
- Add IP/hostname and port configuration fields
- Implement auto-connect on boot when FluidNC enabled
- Implement immediate connect/disconnect when settings change
- Fix watchdog timeout during WiFi connection (add esp_task_wdt_reset)
- Move sendJsonError() to web_utils.h for cleaner main.cpp
- Update API handlers to use standardized JSON error responses

Tested: FluidNC connects successfully via mDNS hostname
```

---

## Session 2 (Continued): Phase 4 - ETag Caching Implementation

### Task 3: ETag HTTP Caching ✅ COMPLETE

**Completed Features:**
1. **ETag Generation Function**
   - **Location:** `src/web/web_utils.h:55-61`
   - Uses MD5Builder for content hashing
   - Returns properly quoted ETag string (RFC 7232 compliant)
   - Fast hash generation for content verification

2. **Conditional GET Support**
   - **Function:** `checkETag()` in `src/web/web_utils.h:66-83`
   - Checks If-None-Match header from client
   - Returns 304 Not Modified when content matches
   - Includes ETag header in 304 response

3. **Unified HTML Response Helper**
   - **Function:** `sendHTMLWithETag()` in `src/web/web_utils.h:88-99`
   - Automatic ETag checking and generation
   - Cache-Control header (public, max-age=300)
   - Works for both HTML and JSON responses

4. **Updated All HTML Handlers:**
   - `handleRoot()` - Main dashboard
   - `handleSettings()` - User settings
   - `handleAdmin()` - Admin/calibration
   - `handleWiFi()` - WiFi configuration
   - `handleSensors()` - Sensor configuration
   - `handleDriverSetup()` - Driver assignment
   - `handleAPIConfig()` - JSON config API
   - `handleAPIStatus()` - JSON status API

**Technical Implementation:**

```cpp
// ETag generation (MD5 hash of content)
String generateETag(const String& content) {
    MD5Builder md5;
    md5.begin();
    md5.add(content);
    md5.calculate();
    return "\"" + md5.toString() + "\"";
}

// Automatic 304 response when content matches
sendHTMLWithETag(server, "text/html", htmlContent);
```

**Performance Benefits:**
- **Bandwidth Reduction:** 304 responses are ~200 bytes vs. 5-50KB HTML
- **Faster Page Loads:** Browser uses cached content when available
- **Reduced Server Load:** Less content generation and transmission
- **Smart Caching:** ETag updates when content changes (config updates, sensor readings, etc.)

**Cache Strategy:**
- HTML pages cached for 5 minutes (Cache-Control: max-age=300)
- ETag validates cache freshness on every request
- Dynamic content (sensor temps, IP addresses) causes ETag change
- Browser automatically sends If-None-Match on subsequent requests

**Example HTTP Flow:**

**First Request:**
```
GET /settings HTTP/1.1

HTTP/1.1 200 OK
ETag: "a1b2c3d4e5f6..."
Cache-Control: public, max-age=300
Content-Length: 15234

<html>...</html>
```

**Subsequent Request (content unchanged):**
```
GET /settings HTTP/1.1
If-None-Match: "a1b2c3d4e5f6..."

HTTP/1.1 304 Not Modified
ETag: "a1b2c3d4e5f6..."
Content-Length: 0
```

**Testing Procedure:**
1. Build and upload firmware
2. Open browser DevTools (Network tab)
3. Load any page (e.g., `/settings`)
4. Verify ETag header in response
5. Reload page (F5 or Ctrl+R)
6. Verify 304 Not Modified response
7. Change a setting and save
8. Reload page - should see 200 OK (new ETag)

---

## Session 2 (Continued): Phase 5 - Code Cleanup ✅ COMPLETE

### Cleanup Tasks Completed:

**1. Removed Unused Web Routes**
- **Deleted:** `/api/reload-screens` endpoint (GET and POST)
- **Reason:** JSON screen rendering disabled in Phase 2
- **Handler removed:** `handleAPIReloadScreens()` function
- **Location:** `src/main.cpp:566-575, 922-924, 940-946`

**2. Removed Unused Include Files**
- **Deleted includes:**
  - `#include "display/screen_renderer.h"` - JSON screen rendering (Phase 2 disabled)
  - `#include <LovyanGFX.hpp>` - Already included via display.h
- **Retained includes:**
  - `#include <WiFiClientSecure.h>` - Required by WebSocketsClient library (dependency)
  - ArduinoJson (used directly for JSON APIs)
- **Location:** `src/main.cpp:15-36`

**3. Updated Header Comments**
- **Before:** "FluidDash v0.2.001 - CYD Edition with hard coded Screen Layouts"
- **After:** Comprehensive feature list reflecting current architecture:
  - Standalone operation emphasis
  - Touch-based sensor assignment
  - Optional WiFi and FluidNC
  - ETag caching
  - NVS persistence
- **Location:** `src/main.cpp:1-14`

**4. Cleaned Up Obsolete Code Comments**
- Removed commented-out JSON editor routes (never implemented)
- Updated "PROGMEM HTML" section to "HTML & Web Resources"
- Added note about ETag caching and dual-storage fallback
- Simplified and modernized inline comments
- **Location:** Various locations throughout `src/main.cpp`

**5. Fixed ArduinoJson Deprecation Warning**
- **Changed:** `doc.containsKey("timeout")` → `!doc["timeout"].isNull()`
- **Reason:** ArduinoJson 7 deprecated `containsKey()` method
- **Modern syntax:** Use `.isNull()` to check for key existence
- **Location:** `src/main.cpp:777`

**Code Size Impact:**
- Removed ~15 lines of commented-out code
- Removed 2 unnecessary includes (screen_renderer.h, LovyanGFX.hpp)
- Removed 1 unused function and 2 route registrations
- Fixed 1 deprecated API call
- **Estimated:** ~1KB reduction in compiled firmware size

**Memory Impact:**
- Reduced include overhead
- Removed unused function call overhead
- Cleaner code = easier maintenance

---

## Files Modified Summary (Session 2)

### Core Files:
- `src/main.cpp` - FluidNC connection logic, JSON error handlers, watchdog fixes, ETag-enabled HTML handlers, code cleanup
- `src/sensors/sensors.cpp` - Position mapping fix, watchdog timeout fix
- `src/display/ui_modes.cpp` - Peak temperature tracking fix
- `src/web/web_utils.h` - JSON error helper function, ETag generation and caching functions

### Web Interface:
- `data/web/settings.html` - FluidNC configuration card

### Documentation:
- `TESTING_CHECKLIST.md` (renamed from TOMORROW.md) - Updated testing status, documented bugs and fixes
- `Phased_Refactoring_Plan_2025-11-27_0932.md` - Updated with Phase 4 progress and Phase 8 (touchscreen) deferred
- `PROGRESS_LOG.md` - Comprehensive development history

---

## Current Phase Status

**✅ COMPLETED:**
- Phase 1: Storage System & HTML Integration
- Phase 2: JSON Screen Rendering Disabled
- Phase 7: Temperature Sensor Naming & Driver Assignment
- Phase 4 - Task 1: ETag HTTP Caching ✅
- Phase 4 - Task 2: Captive Portal (removed - not needed)
- Phase 4 - Task 3: JSON Error Responses ✅
- Phase 5: Code Cleanup & Documentation ✅
- FluidNC Configuration UI (essential feature) ✅

**⏳ REMAINING TASKS:**
- Phase 4 - Task 4: WebSocket Keep-Alive (LOW priority, optional - may skip)
- Phase 6: Final Testing & Validation

**🔮 DEFERRED TO FUTURE:**
- Phase 8: Touchscreen Navigation (after Phase 6)

---

## Known Issues

### Active Issues:
- None currently

### Resolved Issues:
1. ✅ Sensor position mapping bug (Session 2)
2. ✅ Watchdog timeout during touch detection (Session 2)
3. ✅ Watchdog timeout during WiFi connection (Session 2)
4. ✅ Missing FluidNC configuration UI (Session 2)

---

## Testing Notes

### Hardware Tested:
- ✅ ESP32-2432S028 (CYD)
- ✅ 4× DS18B20 temperature sensors
- ✅ DS3231 RTC
- ✅ SD card + LittleFS fallback
- ✅ WiFi connectivity (STA mode)
- ✅ FluidNC WebSocket connection

### Not Yet Tested:
- Touchscreen (not implemented)
- Long-term stability (24-hour test deferred to Phase 6)
- Memory leak detection (deferred to Phase 6)

---

## Next Steps

1. **Immediate:** ETag caching implementation (Phase 4 Task 1)
2. **Then:** WebSocket keep-alive (optional, if time permits)
3. **After Phase 4:** Code cleanup (Phase 5)
4. **Final:** Comprehensive testing (Phase 6)
5. **Future:** Touchscreen navigation (new phase)

---

## Development Notes

### Coding Standards Followed:
- ✅ Keep main.cpp lean (extract utilities to appropriate files)
- ✅ Use `char[]` instead of `String` for sensor mappings
- ✅ Feed watchdog timer during long operations (`esp_task_wdt_reset()`)
- ✅ Non-blocking patterns maintained
- ✅ Proper error handling with return value checks
- ✅ Named conventions: camelCase for functions/variables

### Architecture Principles:
- **Primary:** Standalone temperature/PSU monitoring (works without WiFi)
- **Optional:** WiFi for web configuration
- **Optional:** FluidNC integration (must be explicitly enabled)

### Build System:
- PlatformIO 3.x
- Arduino ESP32 Core 6.8.0
- LovyanGFX 1.1.16
- ArduinoJson 7.2.0

---

**End of Progress Log**
