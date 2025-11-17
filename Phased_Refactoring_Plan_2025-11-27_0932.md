Excellent! I've gathered all the necessary information. Let me create a comprehensive phased plan for your refactoring project.

## Web Server Library Recommendation

**Use Standard WebServer (keep current implementation)**

Both FluidNC uses the **standard ESP32 WebServer library**, NOT AsyncWebServer. Your project already uses this, which is perfect. FluidDial doesn't use a web server at all (it's a pendant controller).

**Why Standard WebServer is better:**

- ✅ More stable and mature
- ✅ Simpler programming model
- ✅ Used by FluidNC (proven in CNC applications)
- ✅ Adequate performance for your use case
- ✅ Lower memory footprint
- ✅ Easier debugging

---

## Current State Summary

**What's Working:**

- Hard-coded screen rendering (monitor, alignment, graph, network modes)
- Sensor reading (temperature, fan, PSU)
- FluidNC WebSocket communication
- Basic web server structure

**What's Broken:**

- Storage system not initialized (`storage.begin()` never called)
- HTML files moved but not connected to web handlers
- JSON screen rendering exists but doesn't load files
- SD card code exists but unused

---

# Phased Refactoring Plan

## Phase 1: Storage System Initialization & HTML Integration

**Goal:** Get the web interface working with the new HTML files

**Tasks:**

1. Remove all SD card initialization code from `storage_manager.cpp`
2. Update `StorageManager` to use LittleFS only (remove SD fallback logic)
3. Initialize LittleFS and storage system in `setup()` in main.cpp
4. Rewrite HTML getter functions (`getMainHTML()`, `getSettingsHTML()`, etc.) to load from LittleFS
5. Test web interface to ensure all pages load correctly
6. Remove commented-out PROGMEM HTML constants from main.cpp

**Files Modified:**

- `src/storage_manager.h/cpp` - Remove SD, simplify to LittleFS only
- `src/main.cpp` - Add storage initialization, rewrite HTML getters
- Verify files in `/data/web/` are properly formatted

**Testing:**

- Access all web pages: `/`, `/settings`, `/admin`, `/wifi`
- Verify HTML renders correctly
- Check browser console for errors

---

## Phase 2: Remove JSON Screen Rendering System

**Goal:** Eliminate JSON-based screen layouts, keep only hard-coded screens

**Tasks:**

1. Remove JSON loading code from `screen_renderer.cpp` (`loadScreenConfig()` function)
2. Remove JSON parsing dependencies from screen renderer
3. Remove `updateDynamicElements()` and JSON-based rendering logic
4. Update `updateDisplay()` in `ui_modes.cpp` to only use hard-coded modes
5. Remove screen layout structures and element types from `config.h` (if not used elsewhere)
6. Clean up unused ArduinoJson usage (if only used for screens)
7. Remove JSON screen files from `/data/screens/` directory (or keep as reference)

**Files Modified:**

- `src/display/screen_renderer.h/cpp` - Remove JSON functions
- `src/display/ui_modes.cpp` - Simplify to hard-coded only
- `src/config/config.h` - Remove ScreenLayout structures (if safe)
- `data/screens/` - Archive or delete JSON files

**Files to Keep:**

- `ui_modes.cpp` - Contains the working hard-coded screens
- JSON usage for config files (API responses, settings, etc.)

**Testing:**

- Verify all 4 display modes work: Monitor, Alignment, Graph, Network
- Test mode switching
- Verify display updates correctly with real sensor data
- Check memory usage improvement

---

## Phase 3: Complete SD Card Removal

**Goal:** Remove all SD card code and dependencies

**Tasks:**

1. Remove `SD.h` include from all files
2. Remove SD card initialization from `storage_manager.cpp`
3. Remove SD-specific methods: `copyToSD()`, SD fallback logic
4. Remove SD card upload handlers from main.cpp (`/upload` route for SD)
5. Simplify `StorageManager` to be a thin wrapper around LittleFS (or remove entirely)
6. Update any documentation or comments referencing SD card
7. Remove SD card pin definitions from `pins.h` (if present)
8. Remove SD upload queue system (`upload_queue.h/cpp`) if only used for SD

**Files Modified:**

- `src/storage_manager.h/cpp` - Simplify or remove class
- `src/main.cpp` - Remove SD upload routes
- `src/upload_queue.h/cpp` - Remove if SD-only
- `src/config/pins.h` - Remove SD pins
- `platformio.ini` - Remove SD library (if any)

**Decision Point:**

- If `StorageManager` becomes just a thin wrapper, consider removing it entirely
- Replace calls like `storage.loadFile()` with direct `LittleFS.open()` calls
- This simplifies code and removes unnecessary abstraction layer

**Testing:**

- Verify project compiles without SD library
- Verify file operations still work (config load/save)
- Check memory usage (should improve)
- Test file uploads to LittleFS (if needed)

---

## Phase 4: Web Server Optimization & Cleanup

**Goal:** Optimize web server based on FluidNC patterns

**Tasks:**

1. Add ETag support for file caching (like FluidNC)
2. Implement proper cookie-based session handling (if authentication needed)
3. Add WebSocket keep-alive pings (10-second interval, like FluidNC)
4. Improve error responses with structured JSON format
5. Add file upload handler for LittleFS (if needed for future config updates)
6. Implement captive portal support for AP mode (like FluidNC)
7. Clean up unused web routes (remove JSON editor routes if not needed)

**Optional Enhancements (from FluidNC):**

- Pre-compute file hashes for ETags at startup
- Add motion-blocking setting (prevent web access during critical operations)
- Implement If-None-Match header handling for 304 responses
- Add session timeout enforcement (6 hours)

**Files Modified:**

- `src/main.cpp` - Enhance route handlers
- `src/network/network.cpp` - Add WebSocket improvements
- `src/web/web_utils.h` - Add caching utilities

**Testing:**

- Test browser caching (check 304 responses)
- Verify WebSocket stability over long periods
- Test AP mode captive portal
- Check concurrent request handling

---

## Phase 5: Code Cleanup & Documentation

**Goal:** Clean up remnants and document changes

**Tasks:**

1. Remove unused includes across all files
2. Remove unused variables and functions
3. Update comments to reflect new architecture
4. Remove `/api/reload-screens` endpoint (no longer needed without JSON)
5. Clean up debug Serial.print statements
6. Update `html_to_spiffs.md` documentation
7. Create migration notes documenting changes
8. Review and optimize memory usage
9. Check for any remaining references to removed systems

**Files Modified:**

- All source files - cleanup
- `data/html_to_spiffs.md` - Update documentation
- Add `MIGRATION_NOTES.md` - Document changes

**Memory Optimization:**

- Review static allocations
- Check for memory leaks
- Verify heap usage under load
- Test long-running stability

**Testing:**

- Full system test of all features
- Memory leak testing (run for extended period)
- Performance benchmarking
- User acceptance testing

---

## Phase 6: Final Testing & Validation

**Goal:** Comprehensive testing of refactored system

**Test Scenarios:**

1. **Web Interface:**
   
   - Load all pages in multiple browsers
   - Test settings save/load
   - Test admin calibration functions
   - Test WiFi configuration

2. **Display System:**
   
   - Test all 4 display modes
   - Verify smooth transitions
   - Test with real sensor data
   - Check display refresh rates

3. **Network:**
   
   - Test STA mode (connect to existing WiFi)
   - Test AP mode (create access point)
   - Test FluidNC WebSocket connection
   - Test long-duration connections

4. **Persistence:**
   
   - Test config save/restore
   - Test reboot recovery
   - Test factory reset
   - Verify LittleFS reliability

5. **Resource Usage:**
   
   - Monitor heap usage over 24 hours
   - Check CPU usage under load
   - Verify no memory leaks
   - Test watchdog stability

---

## Risk Assessment

**Low Risk:**

- Phase 1 (HTML integration) - Just reconnecting existing files
- Phase 4 (optimization) - Enhancements, not breaking changes

**Medium Risk:**

- Phase 2 (remove JSON screens) - Well-isolated system
- Phase 5 (cleanup) - Careful review needed

**High Risk:**

- Phase 3 (remove SD) - Affects multiple systems, need careful dependency analysis

**Mitigation:**

- Create git branch before each phase
- Test thoroughly after each phase
- Keep rollback option available
- Document all changes

---

## Estimated Timeline

- **Phase 1:** 2-4 hours
- **Phase 2:** 3-5 hours
- **Phase 3:** 2-3 hours
- **Phase 4:** 4-6 hours
- **Phase 5:** 2-3 hours
- **Phase 6:** 4-6 hours

**Total:** 17-27 hours of development + testing

---

## Dependencies Between Phases

- Phase 1 must complete before Phase 3 (need working storage before removing SD)
- Phase 2 can run parallel to Phase 1 (independent systems)
- Phase 3 depends on Phase 1 (storage must work first)
- Phase 4 can start after Phase 1 (needs working web server)
- Phase 5 after all others
- Phase 6 is final validation

**Recommended Order:**

1. Phase 1 (get web working)
2. Phase 2 (remove JSON - can overlap with Phase 1)
3. Phase 3 (remove SD - after storage working)
4. Phase 4 (optimize web server)
5. Phase 5 (cleanup)
6. Phase 6 (final testing)

---

## Success Criteria

**Phase 1 Success:**

- ✅ All web pages load from LittleFS HTML files
- ✅ No references to PROGMEM HTML constants
- ✅ Web interface fully functional

**Phase 2 Success:**

- ✅ All JSON screen rendering code removed
- ✅ Hard-coded screens work perfectly
- ✅ Display modes function identically to before
- ✅ Reduced memory usage

**Phase 3 Success:**

- ✅ No SD card code remains
- ✅ LittleFS-only storage works reliably
- ✅ All file operations functional
- ✅ Project compiles without SD library

**Overall Success:**

- ✅ Web interface fully functional
- ✅ Display system working with hard-coded screens only
- ✅ LittleFS-only storage (no SD card)
- ✅ Standard WebServer (no AsyncWebServer)
- ✅ Cleaner, more maintainable codebase
- ✅ Lower memory usage
- ✅ Improved stability

---

This plan provides a structured approach to completely refactor your project while minimizing risk. Each phase has clear goals, specific tasks, and defined success criteria. Would you like me to proceed with Phase 1, or would you like to modify the plan first?
