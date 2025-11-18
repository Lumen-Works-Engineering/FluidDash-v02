# FluidDash v02 - Tomorrow's Work Plan

**Date:** 2025-01-18
**Current Status:** Phase 1, 2, 7, and Driver Assignment System COMPLETE ✅
**Focus:** Hardware testing and validation

---

## Quick Start: What to Do First

### 1. Build and Upload (PRIORITY 1)

```bash
# Build firmware
pio run -e esp32dev

# Upload firmware to device
pio run -e esp32dev -t upload

# Upload filesystem (HTML files)
pio run -e esp32dev -t uploadfs

# Monitor serial output
pio device monitor -b 115200
```

**Expected Boot Messages:**
- RTC initialization (or "RTC not found")
- Temperature sensors discovered (should show 4 sensors)
- WiFi connection or AP mode entry
- Web server started
- IP address displayed

---

## 2. Test Driver Assignment Workflow

### Step-by-Step Testing:

1. **Access the device:**
   - If WiFi connected: Navigate to `http://<device-ip>/`
   - If AP mode: Connect to AP "FluidDash-XXXX" → `http://192.168.4.1/`

2. **Navigate to Driver Setup:**
   - Click **"🎯 Driver Setup"** button on main page
   - Should load `/driver_setup` page showing 4 driver positions

3. **Assign X-Axis sensor:**
   - Click **"🔍 Detect Sensor"** for X-Axis
   - Status should show: "Touch detection started for X-Axis..."
   - Touch/heat the X-Axis driver sensor (body heat works, wait 3-5 seconds)
   - Should see: "✅ X-Axis sensor detected and assigned!"
   - Sensor UID should appear on the card
   - Temperature should display

4. **Repeat for remaining positions:**
   - Y-Left (position 1)
   - Y-Right (position 2)
   - Z-Axis (position 3)

5. **Verify LCD display:**
   - Check LCD monitor mode
   - Should show friendly names: "X-Axis:", "Y-Left:", "Y-Right:", "Z-Axis:"
   - Temperature values should match assigned sensors

6. **Test persistence:**
   - Reboot device
   - Verify assignments still present on `/driver_setup` page
   - Verify LCD still shows correct friendly names

7. **Test Clear functionality:**
   - Click **"Clear"** button for one position
   - Confirm assignment is removed
   - Re-assign to verify workflow still works

---

## 3. Verify Sensor Configuration Page

Navigate to `/sensors` page:

1. Click **"🔍 Scan for Sensors"**
   - Should discover 4 sensors
   - Should show UIDs and current temperatures

2. Click **"🔄 Refresh Temperatures"**
   - Temperatures should update
   - **Known Issue:** May need to re-scan to see updates (minor bug, low priority)

3. Test touch detection:
   - Click **"🔍 Detect This Sensor"** for any sensor
   - Touch that sensor
   - Should confirm correct sensor detected

4. Test sensor naming:
   - Enter friendly name (e.g., "X-Axis Motor")
   - Enter alias (e.g., "temp0")
   - Add notes (optional)
   - Click **"💾 Save Configuration"**
   - Should see success message
   - Verify persistence across reboot

---

## 4. Check for Issues

**Monitor serial output for:**
- ❌ Watchdog timeouts (should NOT happen)
- ❌ NVS errors
- ❌ Temperature read errors (-127°C indicates sensor not found)
- ❌ Memory warnings (free heap should stay stable)

**Test error conditions:**
- What happens if detection times out (don't touch any sensor)?
  - Should show: "Touch detection timed out"
  - Should re-enable detect buttons
- What happens if wrong sensor is touched?
  - Different UID should be assigned (can use Clear to fix)

---

## Success Criteria ✅ - ALL PASSED!

Before proceeding to Phase 4, confirm:

- ✅ All 4 driver sensors successfully assigned to positions - **WORKING** (after bug fixes)
- ✅ LCD displays friendly names correctly - **WORKING** (position mapping fixed)
- ✅ Assignments persist across reboots - **WORKING**
- ✅ Temperature readings are accurate - **WORKING**
- ✅ Touch detection workflow is intuitive and works reliably - **WORKING** (watchdog fixed)
- ✅ No watchdog timeouts or crashes - **WORKING** (explicit watchdog reset added)
- ✅ Timeout handling works - **WORKING** (30-second timeout verified)
- ⏳ Free heap remains stable (check with serial monitor) - **NOT TESTED YET** (deferred to Phase 6)

## Issues Found During Testing (2025-01-18)

### Issue #1: Sensor Position Mapping Bug ✅ FIXED & VERIFIED
**Status:** Fixed and tested successfully
**Location:** [src/sensors/sensors.cpp:129-141](src/sensors/sensors.cpp#L129-L141)

**Problem:** Temperatures were stored using `sensorMappings[]` array index instead of `displayPosition` field, causing sensors to appear in wrong positions on LCD.

**Example:**
- YL sensor assigned to position 1, but appeared at position 0 (labeled "X:")
- Z sensor assigned to position 3, but appeared at position 1 (labeled "YL:")

**Fix Applied:**
- Changed `temperatures[i] = temp` to `temperatures[pos] = temp` where `pos = sensorMappings[i].displayPosition`
- Also fixed peak temperature tracking in [src/display/ui_modes.cpp:178-212](src/display/ui_modes.cpp#L178-L212)

---

### Issue #2: Watchdog Timeout During Touch Detection ✅ FIXED & VERIFIED (Iteration 2)
**Status:** Fixed and tested successfully (30s detection runs without timeout)
**Location:** [src/sensors/sensors.cpp:439-500](src/sensors/sensors.cpp#L439-L500)

**Problem:** `detectTouchedSensor()` runs for 30 seconds blocking the web server handler, preventing main `loop()` from executing and feeding the watchdog timer.

**Root Cause:** The web API endpoint `/api/sensors/detect` calls `detectTouchedSensor()` synchronously, blocking the entire main loop for up to 30 seconds. The ESP32 watchdog timer has a 10-second timeout.

**Error Message:**
```
E (28936) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:
E (28936) task_wdt:  - loopTask (CPU 1)
```

**Attempted Fix #1 (Insufficient):**
- Replaced `delay(750)` with non-blocking wait loop: `while (millis() - start < 750) { delay(50); yield(); }`
- Result: Timeout delayed from 10s to 29s, but still crashed

**Final Fix #2 (Working):**
- Added `#include <esp_task_wdt.h>` to explicitly reset watchdog timer
- Added `esp_task_wdt_reset()` calls in three locations:
  1. During baseline temperature conversion wait (line 455)
  2. During each temperature reading wait in main loop (line 478)
  3. After each sensor check cycle (line 495)
- Now explicitly feeds watchdog every 50ms during entire 30-second detection window

---

## Testing Completed (Lines 1-247) ✅ ALL TESTS PASSED

✅ **Lines 11-25:** Build and upload workflow documented
✅ **Lines 36-75:** Driver assignment workflow tested - **found bugs, now fixed and verified**
✅ **Lines 77-102:** Sensor configuration page tested - **working**
✅ **Lines 104-132:** Issue detection - **found 2 critical bugs, both fixed**
✅ **Lines 213-247:** Bug fixes verified successfully:
  - Touch detection runs for full 30 seconds without watchdog timeout
  - Timeout handling works correctly (graceful timeout after 30s)
  - Sensor positions display correctly on LCD (X-Axis, Y-Left, Y-Right, Z-Axis)
  - Friendly names match physical sensor locations
  - All 4 driver sensors successfully assigned and persisted across reboots

---

## If Everything Works: Next Steps

**Phase 4: Web Server Optimization** (Priority 2)
- Add ETag caching for static files
- Implement captive portal for AP mode
- Add WebSocket keep-alive pings
- Improve error handling with structured JSON responses

**Phase 5: Code Cleanup** (Priority 2)
- Remove unused includes and functions
- Clean up debug Serial.print statements
- Remove unused `/api/reload-screens` endpoint
- Update comments to reflect new architecture
- Review and optimize memory usage

**Phase 6: Final Testing** (Priority 3)
- 24-hour stability test
- Memory leak detection
- Comprehensive feature testing across all modes

---

## If Issues Found: Debugging Steps

### Issue: Sensors not discovered
- Check serial output for OneWire errors
- Verify GPIO pin 21 is correct OneWire pin
- Check physical connections
- Verify 4.7kΩ pull-up resistor on data line

### Issue: Touch detection doesn't work
- Verify sensors are actually changing temperature
- Check serial output for baseline temps
- Try longer touch duration (5-10 seconds)
- Verify threshold (currently 1°C) is appropriate

### Issue: Assignments don't persist
- Check serial output for NVS errors
- Verify `saveSensorConfig()` is called after assignment
- Check available NVS space

### Issue: LCD doesn't show friendly names
- Verify `getSensorMappingByPosition()` returns valid pointer
- Check serial output for sensor mapping data
- Verify display update is called after assignment

---

## Files to Review if Debugging Needed

**Driver Assignment System:**
- [src/sensors/sensors.h](src/sensors/sensors.h) - Lines 14, 81-88
- [src/sensors/sensors.cpp](src/sensors/sensors.cpp) - Lines 337-381 (NVS), 484-560 (position mgmt)
- [src/main.cpp](src/main.cpp) - Lines 748-856 (API endpoints)
- [src/display/ui_modes.cpp](src/display/ui_modes.cpp) - Lines 168-212 (display rendering)
- [data/web/driver_setup.html](data/web/driver_setup.html) - Full file (web UI)

**Sensor Configuration:**
- [src/sensors/sensors.cpp](src/sensors/sensors.cpp) - Lines 228-482 (discovery, touch detection, NVS)
- [data/web/sensor_config.html](data/web/sensor_config.html) - Full file (web UI)

---

## Documentation References

- **Full refactoring plan:** [Phased_Refactoring_Plan_2025-11-27_0932.md](Phased_Refactoring_Plan_2025-11-27_0932.md)
- **Project overview:** [CLAUDE.md](CLAUDE.md)
- **Sensor system details:** CLAUDE.md Section 3 (Sensor Management) and Section 8 (Driver Assignment)

---

---

## ⚠️ NEXT STEPS AFTER BUG FIXES (2025-01-18) - ITERATION 2

**Two critical bugs have been fixed (with watchdog fix updated). You need to rebuild and retest:**

### 1. Rebuild Firmware (WITH WATCHDOG FIX #2)
```bash
pio run -e esp32dev -t upload
```

**NOTE:** Previous upload had insufficient watchdog fix. This version adds explicit `esp_task_wdt_reset()` calls.

### 2. Clear Old Sensor Assignments
Since the position mapping was broken, you should start fresh:
- Navigate to `/driver_setup` page
- Click **"Clear"** for all 4 positions
- This ensures clean state for re-testing

### 3. Re-test Touch Detection
- Click **"Detect"** for X-Axis position
- Touch the X-Axis driver sensor (body heat, 3-5 seconds)
- **Expected:** No watchdog timeout, detection completes successfully
- Verify sensor UID appears on card
- Repeat for Y-Left, Y-Right, Z-Axis

### 4. Verify LCD Display
- Check that positions now match correctly:
  - Position 1 (top): "X-Axis:" with correct temp
  - Position 2: "Y-Left:" with correct temp
  - Position 3: "Y-Right:" with correct temp
  - Position 4 (bottom): "Z-Axis:" with correct temp
- Touch each sensor and verify the correct position updates

### 5. Confirm Fixes
- ✅ No watchdog timeouts during detection
- ✅ Sensors appear in correct positions on LCD
- ✅ Friendly names match physical sensor locations
- ✅ Assignments persist across reboots

**Ready to start? Run the rebuild command above and follow the testing workflow!** 🚀
