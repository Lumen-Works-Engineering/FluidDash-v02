# Claude Code Instructions: Extract WebSocket Loop to Network Module

## Project Context
**Project Path**: `C:\Users\john_sparks\Documents\PlatformIO\Projects\FluidDash-v02`

**Objective**: Complete the final refactoring recommendation by extracting WebSocket handling logic from `main.cpp` loop() into the network module. This will reduce main.cpp further and improve code organization.

## Current State
- Main.cpp: 285 lines (excellent, but can be slightly improved)
- WebSocket handling: Currently in main.cpp lines ~242-267 (~25 lines)
- Target: Move to network module for better organization

## Task Overview
Extract the WebSocket connection handling, status polling, and debug output from main.cpp loop() into a dedicated function in the network module.

---

## Step 1: Add Function Declaration to network.h

**File**: `src/network/network.h`

**Action**: Add the following function declaration after the existing function declarations (likely after `parseFluidNCStatus`):

```cpp
// WebSocket loop handling (call from main loop)
void handleWebSocketLoop();
```

**Location**: Add this near the end of the file, before the `#endif` closing guard, with other function declarations.

---

## Step 2: Implement Function in network.cpp

**File**: `src/network/network.cpp`

**Action**: Add the following function implementation at the end of the file (after all existing functions):

```cpp
// ========== WebSocket Loop Handling ==========

void handleWebSocketLoop() {
    // Only process WebSocket if connected to WiFi and connection was attempted
    if (WiFi.status() != WL_CONNECTED || !fluidnc.connectionAttempted) {
        return;
    }

    // Process WebSocket events
    yield();  // Yield before WebSocket operations
    webSocket.loop();
    yield();  // Yield after WebSocket operations

    // Poll for status if connected (FluidNC doesn't have automatic reporting)
    if (fluidnc.connected && (millis() - timing.lastStatusRequest >= cfg.status_update_rate)) {
        if (fluidnc.debugWebSocket) {
            Serial.println("[FluidNC] Sending status request");
        }
        yield();  // Yield before send
        webSocket.sendTXT("?");
        yield();  // Yield after send
        timing.lastStatusRequest = millis();
    }

    // Periodic debug output (only every 10 seconds)
    static unsigned long lastDebug = 0;
    if (fluidnc.debugWebSocket && millis() - lastDebug >= 10000) {
        Serial.printf("[DEBUG] State:%s MPos:(%.2f,%.2f,%.2f,%.2f) WPos:(%.2f,%.2f,%.2f,%.2f)\n",
                      fluidnc.machineState.c_str(),
                      fluidnc.posX, fluidnc.posY, fluidnc.posZ, fluidnc.posA,
                      fluidnc.wposX, fluidnc.wposY, fluidnc.wposZ, fluidnc.wposA);
        lastDebug = millis();
    }
}
```

**Location**: Append to the end of network.cpp, creating a new section header for clarity.

---

## Step 3: Replace WebSocket Code in main.cpp

**File**: `src/main.cpp`

**Action**: Find the WebSocket handling block in the `loop()` function and replace it with a single function call.

**Find this code** (approximately lines 242-267):

```cpp
  // WebSocket handling (only if connection was attempted)
  if (WiFi.status() == WL_CONNECTED && fluidnc.connectionAttempted) {
      yield();  // Yield before WebSocket operations
      webSocket.loop();
      yield();  // Yield after WebSocket operations

      // Always poll for status - FluidNC doesn't have automatic reporting
      if (fluidnc.connected && (millis() - timing.lastStatusRequest >= cfg.status_update_rate)) {
          if (fluidnc.debugWebSocket) {
              Serial.println("[FluidNC] Sending status request");
          }
          yield();  // Yield before send
          webSocket.sendTXT("?");
          yield();  // Yield after send
          timing.lastStatusRequest = millis();
      }

      // Periodic debug output (only every 10 seconds now)
      static unsigned long lastDebug = 0;
      if (fluidnc.debugWebSocket && millis() - lastDebug >= 10000) {
          Serial.printf("[DEBUG] State:%s MPos:(%.2f,%.2f,%.2f,%.2f) WPos:(%.2f,%.2f,%.2f,%.2f)\n",
                        fluidnc.machineState.c_str(),
                        fluidnc.posX, fluidnc.posY, fluidnc.posZ, fluidnc.posA,
                        fluidnc.wposX, fluidnc.wposY, fluidnc.wposZ, fluidnc.wposA);
          lastDebug = millis();
      }
  }
```

**Replace with**:

```cpp
  // Handle WebSocket connection and status polling
  handleWebSocketLoop();
```

**Result**: The loop() function in main.cpp becomes cleaner with a single, descriptive function call.

---

## Step 4: Verify Compilation

**Action**: Compile the project to ensure no errors were introduced.

**Command**: 
```bash
pio run
```

**Expected Result**: Clean compilation with no errors or warnings related to the changes.

---

## Step 5: Verify Includes (if needed)

**File**: `src/main.cpp`

**Action**: Ensure that `network/network.h` is included at the top of main.cpp (it should already be there).

**Expected Include** (should already exist around line 23):
```cpp
#include "network/network.h"
```

**Verification**: If the include is missing, add it with the other includes near the top of the file.

---

## Expected Results After Completion

### main.cpp Changes
- **Before**: ~285 lines
- **After**: ~260 lines (25 lines removed, 1 line added)
- **Improvement**: More focused, cleaner loop() function

### network.cpp Changes
- **Added**: ~35 lines (new function with documentation)
- **Benefit**: All network-related logic now consolidated in one module

### Code Organization Benefits
1. ✅ **Better Separation of Concerns**: Network operations in network module
2. ✅ **Improved Readability**: main.cpp loop() is easier to understand
3. ✅ **Easier Testing**: Network module can be tested independently
4. ✅ **Maintainability**: WebSocket changes only need to touch network module

---

## Testing Checklist

After making these changes, verify the following functionality:

- [ ] Project compiles without errors
- [ ] Device boots normally
- [ ] Web server accessible (if WiFi connected)
- [ ] FluidNC WebSocket connection works (if enabled)
- [ ] Status requests sent every `cfg.status_update_rate` milliseconds
- [ ] Debug output appears if `fluidnc.debugWebSocket` is true
- [ ] Machine position updates display correctly
- [ ] No watchdog timer resets during WebSocket operations

---

## Additional Notes

### Why This Refactoring Matters

1. **Single Responsibility Principle**: Each module has one clear purpose
2. **Reduced Coupling**: main.cpp doesn't need to know WebSocket details
3. **Future Enhancements**: Easy to add more network features without touching main.cpp
4. **Code Navigation**: Developers know where to look for network-related code

### Related Files Modified
- `src/network/network.h` - Function declaration added
- `src/network/network.cpp` - Function implementation added
- `src/main.cpp` - WebSocket code replaced with function call

### No Changes Needed To
- `src/state/global_state.h` - Already has all needed state structures
- `src/state/global_state.cpp` - No changes required
- Other modules remain unchanged

---

## Success Criteria

✅ **Task is complete when:**
1. Code compiles successfully
2. main.cpp is ~260 lines (reduced from 285)
3. handleWebSocketLoop() function exists in network module
4. WebSocket functionality works identically to before
5. No new warnings or errors introduced

---

## Rollback Plan (if needed)

If issues arise, simply:
1. Revert the changes to main.cpp (restore original WebSocket block)
2. Remove handleWebSocketLoop() from network.cpp
3. Remove declaration from network.h
4. Recompile

The changes are isolated and can be easily reversed if needed.

---

## Final Project Metrics (After This Task)

| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| main.cpp lines | 285 | ~260 | 8.8% reduction |
| Loop function lines | ~50 | ~25 | 50% reduction |
| WebSocket in main.cpp | 25 lines | 1 line | 96% reduction |
| Network module completeness | 90% | 100% | Fully consolidated |

---

**Document Version**: 1.0  
**Created**: 2025-11-19  
**Project**: FluidDash-v02  
**Last Refactoring Step**: Extract WebSocket Loop to Network Module
