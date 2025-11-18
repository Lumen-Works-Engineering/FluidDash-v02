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

## Success Criteria ✅

Before proceeding to Phase 4, confirm:

- ✅ All 4 driver sensors successfully assigned to positions
- ✅ LCD displays friendly names correctly
- ✅ Assignments persist across reboots
- ✅ Temperature readings are accurate
- ✅ Touch detection workflow is intuitive and works reliably
- ✅ No watchdog timeouts or crashes
- ✅ Free heap remains stable (check with serial monitor)

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

**Ready to start? Run the build commands above and follow the testing workflow!** 🚀
