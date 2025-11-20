# Phase 6: Final Testing & Validation Plan

**Created:** 2025-01-18
**Status:** Ready to Execute
**Purpose:** Comprehensive validation of all implemented features

---

## Pre-Testing Checklist

**Build and Upload:**
```bash
# 1. Clean build
pio run -e esp32dev -t clean

# 2. Build firmware
pio run -e esp32dev

# 3. Upload firmware
pio run -e esp32dev -t upload

# 4. Upload filesystem (if HTML files changed)
pio run -e esp32dev -t uploadfs

# 5. Monitor serial output
pio device monitor -b 115200
```

**Expected Boot Sequence:**
1. Display splash screen (~1s)
2. Storage initialization (SD + LittleFS)
3. RTC detection
4. Temperature sensor discovery (4× DS18B20)
5. WiFi connection or AP mode entry
6. Web server startup
7. **Total boot time:** 5-8 seconds (no watchdog timeouts)

---

## Test Category 1: Core Functionality (No WiFi Required)

### 1.1 Temperature Monitoring ✅
- [ ] All 4 DS18B20 sensors detected
- [ ] Temperature readings update every second
- [ ] No readings of -127°C (sensor error)
- [ ] Readings are stable (±0.5°C variation)
- [ ] Peak temperatures track correctly

**Serial Monitor Check:**
```
[SENSOR] Discovering DS18B20 sensors...
[SENSOR] Found 4 sensors
[SENSOR] UID 0: 28FF641E8C160450
[SENSOR] UID 1: 28AA32FC5D160468
...
```

### 1.2 PSU Voltage Monitoring ✅
- [ ] PSU voltage displays correctly
- [ ] Min/max tracking works
- [ ] ADC sampling is non-blocking
- [ ] Voltage updates smoothly

### 1.3 Fan Control ✅
- [ ] Fan speed changes with temperature
- [ ] PWM control working
- [ ] Tachometer reads RPM correctly (if 4-wire fan)
- [ ] Fan responds to temperature thresholds

### 1.4 LCD Display ✅
- [ ] All 4 display modes work:
  - [ ] Monitor mode (default)
  - [ ] Alignment mode
  - [ ] Graph mode
  - [ ] Network mode
- [ ] Mode switching via button press
- [ ] No display flickering
- [ ] Text is readable and aligned
- [ ] Colors display correctly

### 1.5 RTC Time/Date ✅
- [ ] RTC detected (or "RTC not found" message)
- [ ] Time displays correctly
- [ ] Date displays correctly
- [ ] Time persists across reboots (if RTC present)

**Pass Criteria:** Device functions as standalone temp/PSU monitor without WiFi

---

## Test Category 2: WiFi Functionality

### 2.1 First Boot (AP Mode) ✅
- [ ] Device creates AP: "FluidDash-Setup"
- [ ] Can connect to AP from phone/laptop
- [ ] Web interface accessible at http://192.168.4.1
- [ ] WiFi configuration page loads
- [ ] Can save WiFi credentials

### 2.2 STA Mode (Normal Operation) ✅
- [ ] Device connects to configured WiFi
- [ ] IP address shown on LCD Network mode
- [ ] mDNS hostname works (if configured)
- [ ] Connection stable (no disconnects)
- [ ] Serial shows: "WiFi Connected! IP: xxx.xxx.xxx.xxx"

### 2.3 AP Mode Re-Entry ✅
- [ ] Button hold >5s enters AP mode
- [ ] Web server starts automatically
- [ ] Can reconfigure WiFi settings
- [ ] Device reconnects after save

**Pass Criteria:** WiFi works in both AP and STA modes, button hold enters setup

---

## Test Category 3: Web Interface

### 3.1 Main Dashboard (/) ✅
- [ ] Page loads correctly
- [ ] Device name displays
- [ ] IP address displays
- [ ] Navigation buttons work
- [ ] No console errors (F12 DevTools)

### 3.2 Settings Page (/settings) ✅
- [ ] All settings load with current values
- [ ] Temperature thresholds can be changed
- [ ] Fan min speed adjustable
- [ ] PSU alert voltages configurable
- [ ] Graph timespan/interval dropdowns work
- [ ] Coordinate decimal places setting works
- [ ] FluidNC integration card visible
- [ ] Save button works
- [ ] Success message displays after save
- [ ] Settings persist after reboot

### 3.3 Admin/Calibration Page (/admin) ✅
- [ ] Page loads
- [ ] Calibration offsets can be entered
- [ ] Save functionality works
- [ ] Offsets apply to temperature readings

### 3.4 Sensor Configuration (/sensors) ✅
- [ ] Sensor discovery button works
- [ ] All 4 sensors discovered with UIDs
- [ ] Temperature refresh works
- [ ] Touch detection works (30s timeout)
- [ ] Friendly names can be assigned
- [ ] Aliases can be assigned
- [ ] Notes can be added
- [ ] Save functionality works
- [ ] Configuration persists in NVS

### 3.5 Driver Setup (/driver_setup) ✅
- [ ] Page loads correctly
- [ ] 4 driver positions shown (X, YL, YR, Z)
- [ ] "Detect" button starts touch detection
- [ ] Touch sensor → auto-assigns to position
- [ ] Auto-naming works ("X-Axis", "Y-Left", etc.)
- [ ] Clear button removes assignment
- [ ] Assignments persist in NVS
- [ ] LCD display shows friendly names
- [ ] Position-based display mapping works

### 3.6 WiFi Configuration (/wifi) ✅
- [ ] Page loads
- [ ] Can scan for networks
- [ ] Can enter SSID/password
- [ ] Save and connect works

**Pass Criteria:** All web pages load and function correctly

---

## Test Category 4: ETag HTTP Caching (NEW!)

### 4.1 ETag Header Verification ✅
**Setup:** Open Browser DevTools (F12) → Network tab

**Test Procedure:**
1. Load `/settings` page
   - [ ] Response has `ETag: "..."` header
   - [ ] Response has `Cache-Control: public, max-age=300` header
   - [ ] Status: 200 OK
   - [ ] Content-Length: ~15KB

2. Reload page (F5 or Ctrl+R)
   - [ ] Request has `If-None-Match: "..."` header
   - [ ] Status: 304 Not Modified
   - [ ] Content-Length: 0 bytes
   - [ ] Page loads from cache (instant)

3. Change a setting and save
4. Reload page
   - [ ] Status: 200 OK (new content)
   - [ ] ETag value changed (new hash)
   - [ ] Updated settings displayed

### 4.2 Test All Pages ✅
Repeat ETag verification for:
- [ ] `/` (main dashboard)
- [ ] `/settings`
- [ ] `/admin`
- [ ] `/sensors`
- [ ] `/driver_setup`
- [ ] `/wifi`
- [ ] `/api/config`
- [ ] `/api/status`

**Pass Criteria:**
- First load: 200 OK with ETag
- Reload: 304 Not Modified
- After change: 200 OK with new ETag
- **Bandwidth reduction:** 95%+ on cached responses

---

## Test Category 5: FluidNC Integration

### 5.1 Configuration UI ✅
- [ ] FluidNC card visible in `/settings`
- [ ] Enable checkbox works
- [ ] IP/hostname field accepts input
- [ ] Port field accepts numbers
- [ ] mDNS hostname works (e.g., "fluidnc.local")
- [ ] Save button works
- [ ] Settings persist in NVS

### 5.2 Connection Testing ✅
**With FluidNC Powered On:**
1. Enable FluidNC integration
2. Enter hostname: "fluidnc.local"
3. Enter port: 81
4. Click Save
   - [ ] Serial shows: "[FluidNC] Enabled via settings - connecting..."
   - [ ] Serial shows: "[FluidNC] Connected to: /ws"
   - [ ] LCD Alignment mode shows "Connected"

**With FluidNC Powered Off:**
1. Enable FluidNC integration
2. Save settings
   - [ ] Connection fails gracefully (no crash)
   - [ ] Serial shows: "[FluidNC] Connection failed"
   - [ ] LCD shows "Disconnected"

**Auto-Connect on Boot:**
1. Reboot device
   - [ ] Serial shows: "[FluidNC] Auto-discover enabled - connecting..."
   - [ ] FluidNC reconnects automatically
   - [ ] LCD shows connection status

**Disable FluidNC:**
1. Uncheck Enable checkbox
2. Save
   - [ ] Serial shows: "[FluidNC] Disabled via settings - disconnecting..."
   - [ ] Connection closes
   - [ ] No reconnection attempts on reboot

**Pass Criteria:** FluidNC optional, config persists, no crashes when unavailable

---

## Test Category 6: Driver Sensor Assignment

### 6.1 Touch Detection Workflow ✅
1. Navigate to `/driver_setup`
2. Click "Detect" for X-Axis
   - [ ] Button shows "Detecting..."
   - [ ] 30-second countdown starts
   - [ ] Touch sensor (body heat on X-Axis driver)
   - [ ] Detection completes in 3-5 seconds
   - [ ] UID displayed on card
   - [ ] Auto-named "X-Axis"
   - [ ] Temperature shown

3. Repeat for Y-Left, Y-Right, Z-Axis
   - [ ] All 4 positions assigned
   - [ ] Each auto-named correctly

### 6.2 LCD Display Integration ✅
1. Check Monitor mode on LCD
   - [ ] Shows "X-Axis: 42°C" (friendly name)
   - [ ] Shows "Y-Left: 38°C"
   - [ ] Shows "Y-Right: 45°C"
   - [ ] Shows "Z-Axis: 40°C"

2. Touch each sensor and verify correct position updates
   - [ ] Touching X-Axis sensor → top position changes
   - [ ] Touching Y-Left → second position changes
   - [ ] Touching Y-Right → third position changes
   - [ ] Touching Z-Axis → bottom position changes

### 6.3 Persistence Testing ✅
1. Assign all 4 sensors
2. Reboot device
   - [ ] Assignments still present on `/driver_setup`
   - [ ] LCD still shows friendly names
   - [ ] Temperatures still map correctly

### 6.4 Clear Functionality ✅
1. Click "Clear" for one position
   - [ ] Assignment removed from UI
   - [ ] LCD shows default label (e.g., "Temp0:")
2. Re-assign sensor
   - [ ] Works correctly
   - [ ] LCD updates to friendly name

**Pass Criteria:** Touch detection reliable, position mapping accurate, NVS persistence works

---

## Test Category 7: Error Handling & Edge Cases

### 7.1 Watchdog Stability ✅
- [ ] No watchdog timeouts during normal operation
- [ ] No timeouts during WiFi connection
- [ ] No timeouts during touch detection (30s)
- [ ] No timeouts during FluidNC connection
- [ ] Clean serial output (no task_wdt errors)

### 7.2 Memory Stability ✅
**Serial Monitor Check:**
```cpp
Serial.printf("[MEM] Free heap: %d bytes\n", ESP.getFreeHeap());
```
- [ ] Initial heap: ~250-300KB
- [ ] After 1 hour: No significant decrease
- [ ] After 4 hours: Stable heap usage
- [ ] No continuous heap degradation

### 7.3 Filesystem Fallback ✅
**Test SD Card Removal:**
1. Remove SD card
2. Reboot device
   - [ ] LittleFS fallback works
   - [ ] HTML pages load from LittleFS
   - [ ] Web interface functional

**Test SD Card Present:**
1. Insert SD card with `/web/` files
2. Reboot device
   - [ ] SD card detected
   - [ ] HTML pages load from SD
   - [ ] Dual-storage working

### 7.4 Network Disconnection ✅
1. Connect to WiFi
2. Disable router WiFi
   - [ ] Device continues temp/PSU monitoring
   - [ ] LCD still updates
   - [ ] No crash
3. Re-enable WiFi
   - [ ] Device reconnects (may require reboot)

### 7.5 Invalid Input Handling ✅
**Settings Page:**
- [ ] Enter temp_low > temp_high → error or swap
- [ ] Enter invalid PSU voltage → validation
- [ ] Enter invalid IP address → error message

**Sensor Config:**
- [ ] Touch detection timeout (30s) → graceful failure
- [ ] Invalid UID format → error message

**Driver Setup:**
- [ ] Assign same sensor to multiple positions → last wins or error
- [ ] Touch detection timeout → buttons re-enable

**Pass Criteria:** No crashes on invalid input, graceful error handling

---

## Test Category 8: Performance & Responsiveness

### 8.1 Display Update Rate ✅
- [ ] LCD updates every 1 second (smooth)
- [ ] No visible lag or stuttering
- [ ] Graph mode updates correctly
- [ ] No screen tearing or artifacts

### 8.2 Web Interface Responsiveness ✅
- [ ] Pages load in <2 seconds (initial)
- [ ] Pages load in <200ms (cached, 304)
- [ ] Button clicks respond immediately
- [ ] AJAX calls complete quickly
- [ ] No browser console errors

### 8.3 Sensor Reading Speed ✅
- [ ] Temperature reads complete in <1s
- [ ] ADC sampling is non-blocking
- [ ] Fan control responds smoothly
- [ ] No delays in main loop

**Pass Criteria:** System responsive, no noticeable delays

---

## Test Category 9: Long-Term Stability (Optional)

### 9.1 24-Hour Burn-In Test
**Setup:**
1. Power on device
2. Connect to WiFi
3. Enable FluidNC (if available)
4. Let run for 24 hours

**Monitor:**
- [ ] No crashes or reboots
- [ ] No watchdog timeouts
- [ ] Memory usage stable (check serial every 4 hours)
- [ ] WiFi connection stable
- [ ] FluidNC connection stable
- [ ] Temperature readings stable
- [ ] Web interface accessible throughout

**Serial Monitoring:**
```bash
# Check free heap every 4 hours
[MEM] Free heap: 287456 bytes (Hour 0)
[MEM] Free heap: 285312 bytes (Hour 4)
[MEM] Free heap: 284896 bytes (Hour 8)
[MEM] Free heap: 284512 bytes (Hour 12)
[MEM] Free heap: 284256 bytes (Hour 16)
[MEM] Free heap: 284128 bytes (Hour 20)
[MEM] Free heap: 284064 bytes (Hour 24)
```
**Acceptable:** <5% heap degradation over 24 hours

**Pass Criteria:** Device runs stable for 24 hours without intervention

---

## Known Issues to Watch For

### From Previous Sessions:
1. **NVS Warnings on First Boot** (Expected)
   ```
   [E] Preferences.cpp: nvs_get_str len fail: dev_name NOT_FOUND
   ```
   - **Status:** Normal behavior, defaults are used

2. **Temperature Reads -127°C**
   - **Cause:** Sensor not found, wiring issue, missing pull-up
   - **Action:** Check OneWire connections

3. **Watchdog Timeout**
   - **Cause:** Blocking operation >10 seconds
   - **Fixed:** WiFi connection, touch detection
   - **Monitor:** Should not occur in Phase 6

---

## Testing Summary Template

After completing all tests, fill out this summary:

**Date Tested:** ___________
**Firmware Version:** v0.2
**Tester:** ___________

### Results:
- [ ] Core Functionality: PASS / FAIL
- [ ] WiFi Functionality: PASS / FAIL
- [ ] Web Interface: PASS / FAIL
- [ ] ETag Caching: PASS / FAIL
- [ ] FluidNC Integration: PASS / FAIL
- [ ] Driver Assignment: PASS / FAIL
- [ ] Error Handling: PASS / FAIL
- [ ] Performance: PASS / FAIL
- [ ] Long-Term Stability: PASS / FAIL / SKIPPED

### Issues Found:
1. ___________________________________________
2. ___________________________________________
3. ___________________________________________

### Recommendations:
1. ___________________________________________
2. ___________________________________________
3. ___________________________________________

---

## Next Steps After Testing

**If All Tests Pass:**
- Mark Phase 6 as COMPLETE ✅
- Update PROGRESS_LOG.md with test results
- Consider deployment to production
- Move to Phase 8 (Touchscreen) if desired

**If Issues Found:**
- Document issues in PROGRESS_LOG.md
- Create bug fix tasks
- Re-test after fixes
- Repeat Phase 6 testing

---

**End of Phase 6 Testing Plan**
