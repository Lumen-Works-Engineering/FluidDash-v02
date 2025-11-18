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

## Files Modified Summary (Session 2)

### Core Files:
- `src/main.cpp` - FluidNC connection logic, JSON error handlers, watchdog fixes
- `src/sensors/sensors.cpp` - Position mapping fix, watchdog timeout fix
- `src/display/ui_modes.cpp` - Peak temperature tracking fix
- `src/web/web_utils.h` - JSON error helper function

### Web Interface:
- `data/web/settings.html` - FluidNC configuration card

### Documentation:
- `TOMORROW.md` - Updated testing status, documented bugs and fixes
- `Phase_4_Implementation_Plan.md` - Created for web server optimization tasks

---

## Current Phase Status

**✅ COMPLETED:**
- Phase 1: Storage System & HTML Integration
- Phase 2: JSON Screen Rendering Disabled
- Phase 7: Temperature Sensor Naming & Driver Assignment
- Phase 4 - Task 2: Captive Portal (removed - not needed)
- Phase 4 - Task 3: JSON Error Responses
- FluidNC Configuration UI (essential feature)

**⏳ IN PROGRESS:**
- Phase 4: Web Server Optimization

**📋 REMAINING TASKS:**
- Phase 4 - Task 1: ETag Caching (HIGH priority)
- Phase 4 - Task 4: WebSocket Keep-Alive (LOW priority, optional)
- Phase 5: Code Cleanup & Documentation
- Phase 6: Final Testing & Validation

**🔮 DEFERRED TO FUTURE:**
- Touchscreen Navigation (new phase after Phase 6)

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
