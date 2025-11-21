# CLAUDE.md - AI Assistant Development Guide

**Project:** FluidDash v02 - ESP32 CYD Edition
**Last Updated:** 2025-01-22
**Version:** 0.3.100
**Status:** Production Ready - Full Feature Set Complete

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Quick Start for AI Assistants](#quick-start-for-ai-assistants)
3. [Repository Structure](#repository-structure)
4. [Development Workflow](#development-workflow)
5. [Code Conventions](#code-conventions)
6. [Architecture & Design Patterns](#architecture--design-patterns)
7. [Key Components](#key-components)
8. [Common Development Tasks](#common-development-tasks)
9. [Testing & Debugging](#testing--debugging)
10. [Gotchas & Known Issues](#gotchas--known-issues)
11. [Best Practices for AI Assistants](#best-practices-for-ai-assistants)

---

## Project Overview

### What is FluidDash?

**FluidDash v02** is a **standalone temperature/PSU monitoring device** for the **ESP32-2432S028 (CYD - Cheap Yellow Display)** designed for CNC machine enclosure monitoring. It **operates independently without WiFi or network connectivity**, providing real-time display of environmental conditions including temperature sensors, PSU voltage, and cooling fan performance.

**Optional features** include WiFi-based web configuration and FluidNC CNC status integration when network connectivity is available.

### Device Operation Priority

1. **PRIMARY (Always Works):** Temperature monitoring (4× DS18B20), PSU voltage monitoring, automatic fan control
2. **OPTIONAL:** WiFi connectivity (AP mode for setup, STA mode when configured)
3. **OPTIONAL:** Web interface for configuration (requires WiFi)
4. **OPTIONAL:** FluidNC CNC status integration (requires WiFi + manual enablement)

### Core Capabilities (No Dependencies)

- **✅ Temperature monitoring** using DS18B20 OneWire sensors (supports 4+)
  - Real-time readings displayed on LCD
  - UID discovery and mapping support
  - Calibration offsets per sensor
- **✅ PSU voltage monitoring** via ADC
  - Configurable alert thresholds
  - Min/max tracking
- **✅ Fan control** with temperature-based PWM and tachometer feedback
  - Automatic speed adjustment based on temperature
  - Configurable minimum speed
  - RPM monitoring
- **✅ 480×320 IPS LCD display** with 5 viewing modes:
  - Monitor: Temperature/PSU/Fan status
  - Graph: Temperature history visualization
  - Alignment: Axis position view (when FluidNC connected)
  - Network: WiFi/connection diagnostics
  - Storage: SD card/SPIFFS/logging status
- **✅ Resistive touchscreen support** (XPT2046)
  - Tap bottom of screen → Cycle display modes
  - Hold top of screen (5s) → Enter WiFi setup mode
  - Visual progress bar during hold
  - Physical button backup (redundancy)
- **✅ Hard-coded screen layouts** with centralized constants (150+ layout values)
- **✅ Dual storage system** (SD card + LittleFS fallback)
- **✅ RTC support** for time/date display (DS3231)

### Optional Capabilities (Require Configuration)

- **📡 WiFi connectivity** (disabled by default)
  - AP mode on first boot (SSID: "FluidDash-Setup")
  - Button hold (>5s) to enter AP mode
  - STA mode when credentials configured
- **🌐 Web-based configuration** interface (HTML/CSS/JavaScript)
  - Settings management
  - Calibration interface
  - System status monitoring
- **🔗 FluidNC integration** (disabled by default, must be explicitly enabled)
  - Real-time CNC monitoring via WebSocket
  - Machine position display
  - Job status tracking
- **📊 Data logging to SD card** (disabled by default)
  - CSV format with RTC timestamps
  - 10-second default interval (configurable 1s-10min)
  - Automatic 10MB file rotation
  - Date-based filenames (fluiddash_YYYYMMDD.csv)
  - Web API for remote control and download

### Hardware Platform

**Target Device:** ESP32-2432S028 (CYD)
- **MCU:** Dual-core ESP32 @ 240MHz (4MB flash)
- **Display:** 480×320 IPS LCD (ST7796 controller, SPI interface)
- **Touchscreen:** XPT2046 resistive touch controller (shared SPI bus)
- **Storage:** SD card slot (VSPI) + embedded LittleFS
- **Peripherals:** 4× DS18B20 temp sensors, DS3231 RTC, fan controller, RGB LED

### Technology Stack

| Component | Technology |
|-----------|------------|
| **Build System** | PlatformIO 3.x |
| **Framework** | Arduino ESP32 Core 6.8.0 |
| **Language** | C++17 |
| **Graphics** | LovyanGFX 1.1.16 |
| **Networking** | WiFi + WebSockets 2.7.1 |
| **Configuration** | ArduinoJson 7.2.0 |
| **Web UI** | Vanilla HTML5/CSS3/JavaScript |

---

## Quick Start for AI Assistants

### First-Time Setup

1. **Check environment:**
   ```bash
   pio --version  # Should be PlatformIO Core 6.x+
   ```

2. **Install dependencies:**
   ```bash
   pio pkg install -e esp32dev
   ```

3. **Build firmware:**
   ```bash
   pio run -e esp32dev
   ```

4. **Upload to device:**
   ```bash
   pio run -e esp32dev -t upload
   ```

5. **Monitor serial output:**
   ```bash
   pio device monitor -b 115200
   ```

### Critical Files to Understand First

When approaching this codebase, read these files in order:

1. **platformio.ini** - Build configuration, dependencies, platform version
2. **src/config/config.h** - Data structures, enums, global configuration
3. **src/config/pins.h** - Hardware pin definitions
4. **src/main.cpp** - Entry point, setup(), loop(), web server handlers
5. **src/sensors/sensors.h** - Sensor interface and data structures
6. **src/display/ui_modes.h** - Display modes and rendering pipeline
7. **src/network/network.h** - WiFi and WebSocket interface

### Before Making Changes

**ALWAYS:**
- ✅ Review `Config` struct in `config.h` before adding configuration options
- ✅ Use `extern` declarations for globals accessed across modules
- ✅ Test with both SD card present and absent (dual-storage fallback)
- ✅ Verify changes don't exceed available RAM (~320KB usable)
- ✅ **Device MUST work without WiFi** - core functionality is standalone
- ✅ **Feed watchdog timer** - Use `yield()` liberally to prevent boot loops
- ✅ Check `Phased_Refactoring_Plan_2025-11-27_0932.md` for implementation status

**NEVER:**
- ❌ Use blocking delays in `loop()` - this is a real-time system
- ❌ Make WiFi/FluidNC mandatory - they are OPTIONAL features
- ❌ Add blocking operations during boot (10s watchdog limit)
- ❌ Allocate large buffers on stack (use heap with `new`/`malloc`)
- ❌ Hardcode IP addresses or credentials
- ❌ Remove watchdog timer yields (causes reboot)
- ❌ Use `String` class for large or frequently-changed strings (use `char[]`)
- ❌ Ignore return values from SD/SPIFFS operations

---

## Refactoring & Architecture History

### Latest Updates (2025-01-20)

**✅ Complete Refactoring Accomplished:**
All planned refactoring phases (1, 2, 4, 5, 7) are now complete. The codebase has been fully modularized with professional-grade architecture. Historical planning documents have been archived to `docs/archive/`.

**✅ Phase 7: Temperature Sensor Management Complete**
- DS18B20 UID-to-name mapping with NVS persistence
- Touch detection for sensor identification (temperature rise method)
- Driver position assignment system (X-Axis, Y-Left, Y-Right, Z-Axis)
- Web-based sensor configuration interface

**✅ Phase 4 & 5: Web Server Optimization & Code Cleanup Complete**
- Web handlers organized and optimized
- Code structure cleaned and documented
- Display modes properly separated

### Earlier Refactoring Work (2025-01-17)

**✅ Phase 1: Storage System & HTML Integration**
- Storage properly initialized via `storage.begin()` in setup()
- HTML files load from filesystem (SD/LittleFS fallback)
- Web handlers use `storage.loadFile()` instead of PROGMEM
- JSON API functions implemented (`getConfigJSON()`, `getStatusJSON()`)

**✅ Phase 2: JSON Screen Rendering Disabled**
- Hard-coded screen rendering exclusively used
- `drawMonitorMode()`, `drawAlignmentMode()`, `drawGraphMode()`, `drawNetworkMode()`
- JSON layout code dormant (not loaded, but still in codebase for future use)
- Display update performance improved

**✅ Critical Fix: Watchdog Boot Loop Resolved**
- Removed blocking `delay()` calls from setup
- WiFi connection timeout reduced to 5 seconds
- FluidNC connection removed from automatic boot sequence
- Boot time now ~5-8 seconds (well under 10s watchdog limit)

**✅ Architecture Refactor: WiFi Made Optional**
- Device runs standalone without WiFi (core functionality)
- 3-tier boot logic: No credentials → AP mode, Credentials → STA mode, Failed → Standalone
- AP mode auto-entry on first boot (SSID: "FluidDash-Setup")
- Button hold (>5s) to manually enter AP mode
- Web server only starts when WiFi available

**✅ FluidNC Integration Made Optional**
- `cfg.fluidnc_auto_discover = false` by default
- No automatic connection attempts
- Must be explicitly enabled via web settings
- WebSocket loop only runs if connection was manually initiated

### Current Architecture State

**Boot Sequence:**
1. Display initialization (~1s)
2. Storage initialization (SD + LittleFS)
3. RTC detection
4. **Temperature sensor initialization (4× DS18B20 discovered)**
5. WiFi check:
   - No credentials? → Enter AP mode, start web server
   - Has credentials? → Attempt connection (5s timeout)
   - Connected? → Start web server
   - Failed? → Continue standalone
6. Draw main interface
7. **Total boot time: ~5-8 seconds**

**Operation Modes:**

| Mode | WiFi | Web Server | FluidNC | Use Case |
|------|------|------------|---------|----------|
| **Standalone** | ❌ | ❌ | ❌ | Default mode - temp/PSU/fan monitoring only |
| **AP Setup** | ✅ (AP) | ✅ | ❌ | First boot or button hold >5s - configure WiFi |
| **WiFi Only** | ✅ (STA) | ✅ | ❌ | Configured WiFi - web interface available |
| **Full Featured** | ✅ (STA) | ✅ | ✅ | WiFi + FluidNC manually enabled |

**Critical Global Flags:**

```cpp
bool inAPMode = false;               // true when in AP mode
bool webServerStarted = false;       // true when web server is running
bool fluidncConnectionAttempted = false;  // true if FluidNC connection initiated
```

### Key Files Modified During Refactoring

**src/main.cpp:**
- Lines 157-161: Replaced blocking delay with yield loop
- Lines 204-273: WiFi refactoring (3-tier boot logic)
- Lines 282-289: Removed automatic FluidNC connection
- Lines 311-352: WebSocket loop guarded by `fluidncConnectionAttempted`
- Lines 686-804: Implemented `getConfigJSON()` and `getStatusJSON()`

**src/config/config.cpp:**
- Line 21: `cfg.fluidnc_auto_discover = false` (standalone default)
- Line 59: Load default = `false` for auto-discovery

**src/display/ui_modes.cpp:**
- Lines 825-853: Updated `enterSetupMode()` to start web server

### Known Issues & Workarounds

**Issue:** First boot shows NVS errors in serial
```
[E] Preferences.cpp: nvs_get_str len fail: dev_name NOT_FOUND
```
**Status:** Expected behavior - NVS is empty on first boot, defaults are used

**Issue:** SD card initialization may fail if display initialized too late
**Workaround:** Display must initialize before SD card (already implemented)

**Issue:** WebSocket can block if FluidNC is unreachable
**Workaround:** WebSocket loop only runs if `fluidncConnectionAttempted` is true (manual initiation only)

### Future Development

**Immediate Next Steps:**
- **Phase 6:** Hardware testing and validation (user-driven when ready)
- **Documentation:** Historical refactoring plans archived to `docs/archive/`

**Future Feature Ideas:**
- **Touchscreen Support:** Add touch-based UI navigation
- **Data Logging:** CSV export of temperature/PSU history to SD card
- **OTA Updates:** Over-the-air firmware update capability
- **Advanced Graphing:** Multi-sensor overlay graphs
- **Alert System:** Configurable temperature/voltage alerts with visual/audio feedback

---

## Repository Structure

```
FluidDash-v02/
├── platformio.ini                 # PlatformIO build configuration
├── CLAUDE.md                      # This file - AI assistant guide (single source of truth)
│
├── docs/                          # Documentation
│   └── archive/                   # Historical planning docs (completed phases)
│       ├── README.md              # Archive index
│       ├── Phased_Refactoring_Plan_2025-11-27_0932.md
│       ├── PROGRESS_LOG.md
│       ├── TESTING_CHECKLIST.md
│       └── (other completed planning docs)
│
├── data/                          # Filesystem data (uploaded to SPIFFS/SD)
│   ├── web/                       # Web interface HTML files
│   │   ├── main.html              # Main dashboard
│   │   ├── settings.html          # User configuration
│   │   ├── admin.html             # Calibration interface
│   │   └── wifi_config.html       # WiFi setup page
│   └── defaults/                  # Default layout backups
│       └── layout_*.json
│
├── src/                           # Main source code
│   ├── main.cpp                   # Firmware entry point (~670 lines)
│   │                              # Contains: setup(), loop(), web handlers
│   │
│   ├── config/                    # Configuration management
│   │   ├── config.h               # Structs: Config, ScreenLayout, ScreenElement
│   │   ├── config.cpp             # Load/save config JSON functions
│   │   └── pins.h                 # GPIO pin definitions for CYD hardware
│   │
│   ├── sensors/                   # Sensor abstraction layer
│   │   ├── sensors.h              # Temperature, PSU, fan interfaces
│   │   └── sensors.cpp            # DS18B20 reading, fan control, ADC sampling
│   │
│   ├── display/                   # Display rendering system
│   │   ├── display.h              # LovyanGFX initialization
│   │   ├── display.cpp            # Display driver setup
│   │   ├── ui_modes.h             # DisplayMode enum, drawing functions
│   │   ├── ui_modes.cpp           # Mode-specific rendering (Monitor, Graph, etc)
│   │   ├── screen_renderer.h      # JSON→Display conversion
│   │   └── screen_renderer.cpp    # Element parsing and drawing
│   │
│   ├── network/                   # Networking layer
│   │   ├── network.h              # WiFi, WebSocket, FluidNC declarations
│   │   └── network.cpp            # WiFi setup, mDNS, WebSocket client
│   │
│   ├── utils/                     # Utility functions
│   │   ├── utils.h                # Memory management, buffers
│   │   └── utils.cpp              # History buffer allocation
│   │
│   ├── web/                       # Web server utilities
│   │   └── web_utils.h            # HTTP helper functions
│   │
│   ├── storage_manager.h          # Dual filesystem abstraction
│   ├── storage_manager.cpp        # SD + SPIFFS unified API
│   ├── upload_queue.h             # File upload queue management
│   └── upload_queue.cpp           # Upload handling logic
│
├── include/                       # (Standard PlatformIO, mostly empty)
├── lib/                           # (External libraries, managed by PlatformIO)
└── .gitignore                     # Git exclusions
```

### Module Responsibility Matrix

| Module | Primary Responsibility | Key Files | External Dependencies |
|--------|------------------------|-----------|------------------------|
| **Config** | Configuration persistence & defaults | config.h, config.cpp | ArduinoJson, SD/SPIFFS |
| **Sensors** | Hardware sensor interfacing | sensors.h, sensors.cpp | DallasTemperature, OneWire |
| **Display** | Screen rendering & UI modes | display.*, ui_modes.*, screen_renderer.* | LovyanGFX |
| **Network** | WiFi, WebSocket, FluidNC comms | network.h, network.cpp | WiFi, WebSockets, mDNS |
| **Storage** | Filesystem abstraction | storage_manager.* | SD, SPIFFS/LittleFS |
| **Web** | HTTP server & REST API | web_utils.h, main.cpp | WebServer |
| **Utils** | Memory management, buffers | utils.* | None |

---

## Development Workflow

### Git Branch Strategy

**Main Branch:** `main` (stable releases)
**Feature Branches:** `feature/*` (manual development)

**Current Working Branch:** `main`

### Commit Message Conventions

Follow conventional commits format:

```
<type>: <short description>

[optional body explaining WHY, not WHAT]

Examples:
feat: Add DS18B20 UID-to-name mapping system
fix: Prevent watchdog timeout during SD card writes
refactor: Extract HTML pages to data/web/ directory
docs: Update sensor management implementation guide
```

**Types:** `feat`, `fix`, `refactor`, `docs`, `test`, `chore`, `perf`

### Pull Request Workflow

1. Create feature branch from `main`
2. Make changes, commit with clear messages
3. Test on hardware (if possible) or verify build
4. Push to remote: `git push -u origin <branch-name>`
5. Create PR with description of changes
6. Await review/merge

### Build & Test Cycle

```bash
# 1. Clean build
pio run -e esp32dev -t clean

# 2. Build firmware
pio run -e esp32dev

# 3. Check for errors/warnings
# Review output for memory usage, warnings

# 4. Upload to device (if hardware available)
pio run -e esp32dev -t upload

# 5. Monitor serial output
pio device monitor -b 115200

# 6. Test changes via web interface
# Open http://<device-ip>/ in browser
```

### Filesystem Data Upload

To upload `/data/` directory to device SPIFFS:

```bash
pio run -e esp32dev -t uploadfs
```

**Note:** This uploads to embedded SPIFFS. For SD card files, manually copy to SD card or use web upload interface.

---

## Code Conventions

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| **Variables** | camelCase | `float psuVoltage = 0;` |
| **Functions** | camelCase | `void readTemperatures();` |
| **Constants** | SCREAMING_SNAKE_CASE | `#define MAX_SENSORS 10` |
| **Enums** | PascalCase (type), SCREAMING_SNAKE_CASE (values) | `enum DisplayMode { MODE_MONITOR };` |
| **Structs** | PascalCase | `struct Config { ... };` |
| **Classes** | PascalCase | `class StorageManager { ... };` |
| **Macros** | SCREAMING_SNAKE_CASE | `#define DEBUG_MODE 1` |
| **Files** | snake_case | `screen_renderer.cpp` |
| **Global vars** | camelCase with context | `bool fluidncConnected;` |

### File Organization

**Header File (.h) Structure:**

```cpp
#ifndef MODULE_NAME_H
#define MODULE_NAME_H

#include <Arduino.h>
// ... other includes

// ========== Constants & Defines ==========
#define MAX_VALUE 100

// ========== Enums & Structs ==========
enum ModuleMode {
    MODE_A,
    MODE_B
};

struct ModuleConfig {
    int setting1;
    float setting2;
};

// ========== Function Declarations ==========
void initModule();
void updateModule();

// ========== External Variables ==========
extern int globalVariable;

#endif // MODULE_NAME_H
```

**Implementation File (.cpp) Structure:**

```cpp
#include "module_name.h"
#include "other_headers.h"

// ========== Module-Local Variables ==========
static int internalCounter = 0;

// ========== Public Functions ==========
void initModule() {
    // Implementation
}

void updateModule() {
    // Implementation
}

// ========== Private/Helper Functions ==========
static void helperFunction() {
    // Internal use only
}
```

### Comment Style

**Section Headers:**
```cpp
// ========== SECTION NAME ==========
```

**Function Documentation:**
```cpp
// Brief description of what function does
// Parameters: param1 - description
// Returns: description of return value
void functionName(int param1) {
    // Implementation
}
```

**Inline Comments:**
```cpp
int value = calculateValue();  // Explain WHY, not WHAT
```

**TODOs:**
```cpp
// TODO: Add error handling for SD card failures
// FIXME: Race condition when WebSocket reconnects
// NOTE: This assumes sensors are already initialized
```

### Variable Declaration Style

**Prefer:**
```cpp
// Initialized at declaration
float temperature = 0.0;
bool isConnected = false;

// Group related variables
float posX = 0, posY = 0, posZ = 0;

// Use arrays for sensor data
float temperatures[4] = {0};
```

**Avoid:**
```cpp
// Uninitialized variables
float temperature;

// Scattered declarations
float posX = 0;
int feedRate = 0;
float posY = 0;
bool connected = false;
```

### Memory Management

**For globals and persistent data:**
```cpp
// Use fixed-size arrays when size is known
float temperatures[4] = {0};

// Use heap for dynamic buffers
float *tempHistory = nullptr;

void allocateBuffer(uint16_t size) {
    if (tempHistory != nullptr) {
        free(tempHistory);
    }
    tempHistory = (float*)malloc(size * sizeof(float));
    if (tempHistory == nullptr) {
        Serial.println("ERROR: Failed to allocate buffer");
    }
}
```

**For temporary data:**
```cpp
// Small buffers: stack allocation
char buffer[64];
snprintf(buffer, sizeof(buffer), "Temp: %.1f", temp);

// Large buffers: heap allocation
JsonDocument doc;  // ArduinoJson manages heap internally
```

### String Handling

**Prefer `char[]` over `String` class:**

```cpp
// GOOD - Fixed buffer, no heap fragmentation
char deviceName[32] = "FluidDash";
snprintf(deviceName, sizeof(deviceName), "FluidDash-%d", id);

// ACCEPTABLE - String for temporary use
String uid = uidToString(sensorUID);  // Convert then use immediately

// AVOID - String concatenation in loops
for (int i = 0; i < 100; i++) {
    String msg = "Sensor " + String(i);  // Heap fragmentation!
}
```

### Error Handling

**Always check return values:**

```cpp
// SD card operations
if (!SD.begin(SD_CS_PIN)) {
    Serial.println("ERROR: SD card initialization failed");
    // Fall back to SPIFFS
}

// File operations
File file = SD.open("/config.json", FILE_READ);
if (!file) {
    Serial.println("ERROR: Failed to open config.json");
    return false;
}

// JSON parsing
DeserializationError error = deserializeJson(doc, file);
if (error) {
    Serial.print("ERROR: JSON parsing failed: ");
    Serial.println(error.c_str());
    return false;
}
```

### Non-Blocking Code Patterns

**NEVER use `delay()` in main loop. Use millis() timing:**

```cpp
// GOOD - Non-blocking delay pattern
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 1000;  // 1 second

void loop() {
    unsigned long now = millis();
    if (now - lastUpdate >= UPDATE_INTERVAL) {
        lastUpdate = now;
        updateDisplay();
    }
}

// BAD - Blocking delay
void loop() {
    updateDisplay();
    delay(1000);  // Blocks entire system!
}
```

---

## Architecture & Design Patterns

### System Architecture

FluidDash uses a **layered architecture** with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────┐
│               USER INTERFACE LAYER                       │
│  Web UI (HTML/CSS/JS) + LCD Display (LovyanGFX)         │
└─────────────────────────────────────────────────────────┘
                         ▼
┌─────────────────────────────────────────────────────────┐
│             APPLICATION LOGIC LAYER                      │
│  Mode Management, Event Handling, State Machines        │
└─────────────────────────────────────────────────────────┘
                         ▼
┌──────────────┬──────────────┬──────────────┬────────────┐
│   Sensors    │   Network    │   Display    │   Config   │
│   Module     │   Module     │   Module     │   Module   │
└──────────────┴──────────────┴──────────────┴────────────┘
                         ▼
┌─────────────────────────────────────────────────────────┐
│          HARDWARE ABSTRACTION LAYER                      │
│  GPIO, SPI, I2C, ADC, PWM, OneWire, WebSocket           │
└─────────────────────────────────────────────────────────┘
```

### Key Design Patterns

#### 1. State Machine (Display Modes)

```cpp
enum DisplayMode {
    MODE_MONITOR,    // CNC status monitoring
    MODE_ALIGNMENT,  // Axis alignment view
    MODE_GRAPH,      // Temperature graphing
    MODE_NETWORK     // Network diagnostics
};

DisplayMode currentMode = MODE_MONITOR;

void drawScreen() {
    switch(currentMode) {
        case MODE_MONITOR:   drawMonitorMode(); break;
        case MODE_ALIGNMENT: drawAlignmentMode(); break;
        case MODE_GRAPH:     drawGraphMode(); break;
        case MODE_NETWORK:   drawNetworkMode(); break;
    }
}
```

#### 2. Dual-Storage Fallback Pattern

```cpp
class StorageManager {
public:
    File openFile(const char* path, const char* mode) {
        // Try SD first
        if (sdAvailable && SD.exists(path)) {
            return SD.open(path, mode);
        }
        // Fallback to SPIFFS
        return SPIFFS.open(path, mode);
    }
};
```

#### 3. Event-Driven WebSocket

```cpp
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            fluidncConnected = false;
            break;
        case WStype_CONNECTED:
            fluidncConnected = true;
            break;
        case WStype_TEXT:
            parseFluidNCStatus((char*)payload);
            break;
    }
}
```

#### 4. JSON-Based Configuration (Removed)

Screen layouts, settings, and sensor configurations are all JSON-driven:

```json
{
  "type": "text_dynamic",
  "x": 10, "y": 50,
  "dataSource": "wposX",
  "label": "X:",
  "decimals": 3,
  "color": "0xFFFF"
}
```

This allows customization without firmware recompilation.

#### 5. Non-Blocking ADC Sampling

```cpp
void sampleSensorsNonBlocking() {
    unsigned long now = millis();
    if (now - lastAdcSample < 5) return;  // Sample every 5ms

    lastAdcSample = now;
    adcSamples[adcCurrentSensor][adcSampleIndex] = analogRead(SENSOR_PIN);

    adcSampleIndex++;
    if (adcSampleIndex >= 10) {
        adcSampleIndex = 0;
        adcCurrentSensor++;
        if (adcCurrentSensor >= 5) {
            adcCurrentSensor = 0;
            adcReady = true;  // Full cycle complete
        }
    }
}
```

### Data Flow Diagrams

#### Temperature Monitoring Flow

```
DS18B20 Sensors (OneWire)
        │
        ▼
readTemperatures()
        │
        ▼
temperatures[4] array
        │
        ├──▶ Display (Monitor Mode)
        ├──▶ Fan Control (PWM)
        ├──▶ History Buffer (Graphing)
        └──▶ Web API (/api/status)
```

#### FluidNC Status Flow

```
FluidNC (WebSocket Server)
        │
        ▼
webSocketEvent() callback
        │
        ▼
parseFluidNCStatus()
        │
        ▼
Global state variables:
  - machineState
  - posX, posY, posZ
  - feedRate, spindleRPM
        │
        ├──▶ Display (Monitor Mode)
        └──▶ Web API (/api/status)
```

---

## Key Components

### 1. Main Loop (main.cpp)

**Location:** `src/main.cpp`

**Primary Responsibilities:**
- Initialize hardware (display, sensors, RTC, WiFi)
- Handle web server requests
- Update sensors and display
- Monitor button presses
- Maintain WebSocket connection

**Critical Global Variables:**
```cpp
extern DisplayMode currentMode;           // Current display mode
extern float temperatures[4];             // DS18B20 readings
extern float psuVoltage;                  // PSU voltage (V)
extern String machineState;               // FluidNC state
extern float posX, posY, posZ, posA;      // Machine coordinates
extern bool fluidncConnected;             // WebSocket status
extern Config cfg;                        // Global configuration
```

**Main Loop Structure:**
```cpp
void loop() {
    server.handleClient();              // Process web requests
    sampleSensorsNonBlocking();         // Non-blocking ADC

    if (millis() - lastUpdate >= 1000) {
        readTemperatures();             // DS18B20 sensors
        controlFan();                   // PWM based on temp
        calculateRPM();                 // Fan tachometer
        updateDisplay();                // Refresh LCD
    }

    handleButtonPress();                // Mode switching
    yield();                            // Prevent watchdog timeout
}
```

### 2. Configuration System (config/*)

**Location:** `src/config/config.h`, `src/config/config.cpp`

**Key Structures:**

```cpp
struct Config {
    // Network
    char device_name[32];
    char fluidnc_ip[16];
    uint16_t fluidnc_port;

    // Temperatures
    float temp_threshold_low;
    float temp_threshold_high;
    float temp_offset_x, temp_offset_yl, temp_offset_yr, temp_offset_z;

    // Display
    uint8_t brightness;
    DisplayMode default_mode;
    uint8_t coord_decimal_places;

    // Graph
    uint16_t graph_timespan_seconds;
    uint16_t graph_update_interval;
};
```

**Usage:**
```cpp
extern Config cfg;  // Declared in main.cpp

// Load from SD/SPIFFS
loadConfig();

// Access settings
if (temperature > cfg.temp_threshold_high) {
    // Alert!
}

// Modify and save
cfg.brightness = 200;
saveConfig();
```

### 3. Sensor Management (sensors/*)

**Location:** `src/sensors/sensors.h`, `src/sensors/sensors.cpp`

**Primary Functions:**

```cpp
// Temperature monitoring
void readTemperatures();              // Read all DS18B20 sensors
float getTempByAlias(const char* alias);  // Get temp by name

// Fan control
void controlFan();                    // Adjust PWM based on temp
void calculateRPM();                  // Read tachometer

// PSU monitoring
void sampleSensorsNonBlocking();      // Non-blocking ADC sampling
void processAdcReadings();            // Average and convert ADC

// Sensor configuration (NVS-based persistence)
void loadSensorConfig();              // Load UID-to-name mapping
void saveSensorConfig();              // Save sensor configuration
String detectTouchedSensor(unsigned long timeout);  // Touch detection

// Driver position management (display mapping)
bool assignSensorToPosition(const uint8_t uid[8], int8_t position);
bool getSensorAtPosition(int8_t position, uint8_t uid[8]);
const SensorMapping* getSensorMappingByPosition(int8_t position);
```

**Data Structures:**
```cpp
struct SensorMapping {
    uint8_t uid[8];           // DS18B20 unique ROM address
    char friendlyName[32];    // "X-Axis Motor"
    char alias[16];           // "temp0"
    bool enabled;
    char notes[64];
    int8_t displayPosition;   // -1=not displayed, 0=X-Axis, 1=Y-Left, 2=Y-Right, 3=Z-Axis, 4+=expansion
};

extern std::vector<SensorMapping> sensorMappings;
```

### 4. Display System (display/*)

**Location:** `src/display/`

**Component Breakdown:**

| File | Responsibility |
|------|----------------|
| `display.h/cpp` | LovyanGFX initialization, hardware setup |
| `ui_modes.h/cpp` | Mode-specific rendering (Monitor, Graph, Alignment) |
| `screen_renderer.h/cpp` | JSON layout parser and element renderer |

**Key Functions:**

```cpp
// Main display control
void drawScreen();           // Draw current mode
void updateDisplay();        // Update dynamic elements

// Mode-specific rendering
void drawMonitorMode();      // CNC status display
void drawGraphMode();        // Temperature graph
void drawAlignmentMode();    // Axis alignment view
void drawNetworkMode();      // WiFi/network diagnostics

// JSON-based rendering
void loadScreenLayouts();                     // Load all JSON layouts
void drawScreenFromLayout(ScreenLayout& layout);  // Render from JSON
void updateDynamicElements(ScreenLayout& layout); // Update values only
```

**JSON Layout System (REMOVED):**

Layouts are defined in `/data/screens/*.json`:

```json
{
  "name": "Monitor Screen",
  "backgroundColor": 0,
  "elements": [
    {
      "type": "text_static",
      "x": 10, "y": 10,
      "label": "FluidDash",
      "textSize": 3,
      "color": 65535
    },
    {
      "type": "coord_value",
      "x": 100, "y": 50,
      "dataSource": "wposX",
      "label": "X:",
      "decimals": 3,
      "color": 2047
    }
  ]
}
```

### 5. Network Layer (network/*)

**Location:** `src/network/network.h`, `src/network/network.cpp`

**Responsibilities:**
- WiFi connection management (STA mode)
- WebSocket client for FluidNC
- mDNS service discovery
- WebSocket event handling

**Key Functions:**

```cpp
void initWiFi();                      // Connect to WiFi (with WiFiManager)
void setupWebSocket(const char* ip, uint16_t port);  // Connect to FluidNC
void handleWebSocketEvents();         // Process WebSocket messages
void parseFluidNCStatus(char* payload);  // Parse status messages
void pollFluidNCStatus();             // Send "?" command for status
```

**WebSocket Protocol:**

FluidDash connects to FluidNC as a WebSocket client:

```
FluidDash                          FluidNC
   │                                  │
   ├──── WebSocket Connect ──────────▶│
   │◀─── Connected (200 OK) ──────────┤
   │                                  │
   ├──── "?" (status request) ───────▶│
   │◀─── "<Idle|WPos:0,0,0|...>" ────┤
   │                                  │
   ├──── "?" (poll every 500ms) ─────▶│
   │◀─── Status update ───────────────┤
```

### 6. Storage Manager (storage_manager.*)

**Location:** `src/storage_manager.h`, `src/storage_manager.cpp`

**Purpose:** Unified API for SD card and SPIFFS with automatic fallback.

**Usage:**

```cpp
StorageManager storage;

// Initialize (tries SD first, falls back to SPIFFS)
storage.begin();

// Open files (tries SD first, then SPIFFS)
File file = storage.openFile("/config.json", FILE_READ);

// Check file existence
if (storage.exists("/screens/monitor.json")) {
    // File found on SD or SPIFFS
}

// List files
std::vector<String> files = storage.listFiles("/screens");
```

**Fallback Priority:**
1. SD card (if available and file exists)
2. SPIFFS/LittleFS (embedded filesystem)

### 7. Web Server & API (main.cpp)

**Location:** `src/main.cpp` (web handlers section)

**REST API Endpoints:**

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/` | GET | Main dashboard (main.html) |
| `/settings` | GET | Settings page |
| `/admin` | GET | Calibration page |
| `/sensors` | GET | Sensor configuration page |
| `/driver_setup` | GET | Driver sensor assignment page |
| `/api/config` | GET/POST | Get/set configuration |
| `/api/status` | GET | System status (JSON) |
| `/api/rtc` | GET/POST | RTC time management |
| `/api/sensors/discover` | GET | Discover all DS18B20 UIDs |
| `/api/sensors/list` | GET | List configured sensors |
| `/api/sensors/temps` | GET | Get current temperatures |
| `/api/sensors/detect` | POST | Detect touched sensor (temp rise) |
| `/api/sensors/save` | POST | Save sensor configuration |
| `/api/drivers/get` | GET | Get driver position assignments |
| `/api/drivers/assign` | POST | Assign sensor UID to position |
| `/api/drivers/clear` | POST | Clear position assignment |
| `/upload` | POST | File upload handler |

**Example API Response:**

```json
GET /api/status

{
  "temperatures": [42.3, 38.1, 45.2, 40.0],
  "psuVoltage": 12.1,
  "fanRPM": 2450,
  "machineState": "Run",
  "position": {"x": 10.234, "y": 5.123, "z": 0.500},
  "fluidncConnected": true
}
```

### 8. Driver Sensor Assignment System

**Purpose:** Map DS18B20 sensors to fixed display positions despite non-deterministic OneWire discovery order.

**Key Concept:** The `displayPosition` field in `SensorMapping` explicitly maps sensor UIDs to positions:
- `0` = X-Axis driver
- `1` = Y-Left driver
- `2` = Y-Right driver
- `3` = Z-Axis driver
- `-1` = Not assigned to a driver position

**User Workflow:**

1. Navigate to `/driver_setup` page
2. Click **"Detect"** button for X-Axis position
3. Touch/heat the physical sensor on X-Axis driver (body heat works, takes 3-5 seconds)
4. System automatically detects temperature rise and assigns that UID to position 0
5. Sensor is auto-named "X-Axis" if no friendly name exists
6. Repeat for Y-Left, Y-Right, Z-Axis positions
7. Assignments persist in NVS across reboots

**Technical Flow:**

```
User clicks "Detect" for X-Axis
    ↓
Frontend: POST /api/sensors/detect (30s timeout)
    ↓
Backend: detectTouchedSensor() monitors temp rise
    ↓
Returns UID of touched sensor
    ↓
Frontend: POST /api/drivers/assign {position: 0, uid: "28FF..."}
    ↓
Backend: assignSensorToPosition() updates displayPosition
    ↓
Saves to NVS via saveSensorConfig()
    ↓
LCD display updated via getSensorMappingByPosition()
```

**Display Integration:**

The LCD now uses position-based lookup instead of array index:

```cpp
// OLD: Array index (unreliable)
float temp = temperatures[0];  // Which sensor is this?

// NEW: Position-based (reliable)
const SensorMapping* sensor = getSensorMappingByPosition(0);  // X-Axis
if (sensor) {
    float temp = getTempByUID(sensor->uid);
    Serial.print(sensor->friendlyName);  // "X-Axis"
}
```

**Files Involved:**
- `data/web/driver_setup.html` - User interface
- `src/sensors/sensors.h/cpp` - Position management functions
- `src/display/ui_modes.cpp` - Position-based display rendering
- `src/main.cpp` - Driver API endpoints

---

## Common Development Tasks

### Adding a New Configuration Option

**Example:** Add a new setting for display timeout.

**Step 1:** Update `Config` struct in `config.h`:

```cpp
struct Config {
    // ... existing fields
    uint16_t display_timeout_seconds;  // New field
};
```

**Step 2:** Update `initDefaultConfig()` in `config.cpp`:

```cpp
void initDefaultConfig() {
    // ... existing defaults
    cfg.display_timeout_seconds = 300;  // 5 minutes default
}
```

**Step 3:** Update `loadConfig()` and `saveConfig()` in `config.cpp`:

```cpp
void loadConfig() {
    // ... existing loading code
    cfg.display_timeout_seconds = doc["display_timeout_seconds"] | 300;
}

void saveConfig() {
    // ... existing saving code
    doc["display_timeout_seconds"] = cfg.display_timeout_seconds;
}
```

**Step 4:** Add UI control in `settings.html`:

```html
<label>Display Timeout (seconds):</label>
<input type="number" id="display_timeout_seconds" min="30" max="3600">
```

**Step 5:** Use the setting in `main.cpp`:

```cpp
if (millis() - lastActivity > cfg.display_timeout_seconds * 1000) {
    // Dim or turn off display
}
```

### Adding a New Display Mode

**Example:** Add a "Debug" mode showing system diagnostics.

**Step 1:** Add to `DisplayMode` enum in `config.h`:

```cpp
enum DisplayMode {
    MODE_MONITOR,
    MODE_ALIGNMENT,
    MODE_GRAPH,
    MODE_NETWORK,
    MODE_DEBUG      // New mode
};
```

**Step 2:** Create drawing function in `ui_modes.cpp`:

```cpp
void drawDebugMode() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);

    tft.setCursor(10, 10);
    tft.print("DEBUG MODE");

    // Show memory usage
    tft.setCursor(10, 40);
    tft.printf("Free Heap: %d bytes", ESP.getFreeHeap());

    // Show WiFi RSSI
    tft.setCursor(10, 70);
    tft.printf("WiFi RSSI: %d dBm", WiFi.RSSI());
}
```

**Step 3:** Add to `drawScreen()` switch in `ui_modes.cpp`:

```cpp
void drawScreen() {
    switch(currentMode) {
        // ... existing modes
        case MODE_DEBUG:
            drawDebugMode();
            break;
    }
}
```

**Step 4:** Add button navigation in `main.cpp`:

```cpp
void handleButtonPress() {
    // Cycle through modes on button press
    if (buttonPressed && millis() - buttonPressStart > 10000) {
        currentMode = (DisplayMode)((currentMode + 1) % 5);  // 5 modes total
        drawScreen();
    }
}
```

### Adding a New Sensor Type

**Example:** Add a humidity sensor (DHT22).

**Step 1:** Add library dependency to `platformio.ini`:

```ini
lib_deps =
    # ... existing deps
    adafruit/DHT sensor library@^1.4.4
```

**Step 2:** Add global variable in `main.cpp`:

```cpp
#include <DHT.h>

#define DHT_PIN 15
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);
float humidity = 0.0;
```

**Step 3:** Initialize in `setup()`:

```cpp
void setup() {
    // ... existing setup
    dht.begin();
}
```

**Step 4:** Read in `loop()`:

```cpp
void loop() {
    // ... existing loop code

    if (millis() - lastSensorUpdate >= 2000) {
        humidity = dht.readHumidity();
        if (isnan(humidity)) {
            humidity = 0.0;  // Error handling
        }
    }
}
```

**Step 5:** Display in `ui_modes.cpp`:

```cpp
void drawMonitorMode() {
    // ... existing display code

    tft.setCursor(10, 200);
    tft.printf("Humidity: %.1f%%", humidity);
}
```

**Step 6:** Add to API in `main.cpp`:

```cpp
server.on("/api/status", HTTP_GET, []() {
    // ... existing JSON
    doc["humidity"] = humidity;
    // ... send response
});
```

### Creating a Custom JSON Screen Layout

**Example:** Create a minimalist "Night Mode" screen.

**Step 1:** Create `/data/screens/night_mode.json`:

```json
{
  "name": "Night Mode",
  "backgroundColor": 0,
  "elements": [
    {
      "type": "text_static",
      "x": 180,
      "y": 20,
      "label": "FluidDash",
      "textSize": 3,
      "color": 31,
      "align": "center"
    },
    {
      "type": "text_dynamic",
      "x": 240,
      "y": 100,
      "dataSource": "machineState",
      "textSize": 4,
      "color": 2016,
      "align": "center"
    },
    {
      "type": "coord_value",
      "x": 20,
      "y": 200,
      "dataSource": "wposX",
      "label": "X:",
      "decimals": 2,
      "textSize": 2,
      "color": 2047
    },
    {
      "type": "temp_value",
      "x": 20,
      "y": 280,
      "dataSource": "temp0",
      "label": "Temp:",
      "decimals": 1,
      "textSize": 2,
      "color": 63488
    }
  ]
}
```

**Step 2:** Load in `main.cpp` or create new mode:

```cpp
ScreenLayout nightLayout;

void setup() {
    // ... existing setup
    loadScreenLayout(&nightLayout, "/screens/night_mode.json");
}

void drawNightMode() {
    if (nightLayout.isValid) {
        drawScreenFromLayout(nightLayout);
    }
}
```

### Debugging WebSocket Connection

**Step 1:** Enable debug output in `main.cpp`:

```cpp
bool debugWebSocket = true;  // Set to true
```

**Step 2:** Check serial monitor for WebSocket events:

```
[WS] Connecting to ws://192.168.1.100:81
[WS] Connected!
[WS] Received: <Idle|WPos:0.000,0.000,0.000|...>
```

**Step 3:** Manually test WebSocket with browser:

```javascript
// Open browser console on same network
const ws = new WebSocket('ws://192.168.1.100:81');
ws.onmessage = (e) => console.log('Received:', e.data);
ws.send('?');  // Request status
```

**Step 4:** Check FluidNC IP configuration:

```cpp
// Verify in /settings page or serial monitor
Serial.println(cfg.fluidnc_ip);  // Should match FluidNC IP
```

---

## Testing & Debugging

### Serial Monitor Output

**Enable verbose logging:**

```cpp
#define DEBUG_MODE 1

#if DEBUG_MODE
  Serial.println("Debug message");
#endif
```

**Common debug points:**

```cpp
void loop() {
    // WebSocket status
    Serial.printf("[WS] Connected: %s\n", fluidncConnected ? "YES" : "NO");

    // Sensor readings
    Serial.printf("[TEMP] X:%.1f YL:%.1f YR:%.1f Z:%.1f\n",
                  temperatures[0], temperatures[1],
                  temperatures[2], temperatures[3]);

    // Memory usage
    Serial.printf("[MEM] Free heap: %d bytes\n", ESP.getFreeHeap());
}
```

### Hardware Testing Checklist

**Before upload:**
- [ ] Code compiles without errors
- [ ] No warnings about stack/heap overflow
- [ ] RAM usage < 80% (check build output)

**After upload:**
- [ ] Serial monitor shows boot messages
- [ ] RTC initializes (or "RTC not found" message)
- [ ] WiFi connects (or AP mode starts)
- [ ] Temperature sensors detected
- [ ] Display shows splash screen
- [ ] Web server accessible at device IP

**Functional tests:**
- [ ] Button cycles through modes
- [ ] Temperature readings update
- [ ] Fan speed changes with temperature
- [ ] WebSocket connects to FluidNC
- [ ] Settings page loads and saves
- [ ] SD card detected (if inserted)

### Common Build Errors

**Error:** `region 'iram0_0_seg' overflowed by N bytes`
**Solution:** Reduce code size by moving strings to PROGMEM or removing debug code.

```cpp
// Instead of:
Serial.println("This is a very long debug message");

// Use:
Serial.println(F("This is a very long debug message"));  // F() macro stores in flash
```

**Error:** `undefined reference to 'functionName'`
**Solution:** Add function declaration to header file or check for typos.

**Error:** `No such file or directory: 'SomeLibrary.h'`
**Solution:** Add library to `platformio.ini` lib_deps.

**Error:** `Sketch too big` or `not enough memory`
**Solution:** Increase flash size in `platformio.ini` or reduce features.

### Debugging Watchdog Timeouts

**Symptom:** Device reboots every few seconds.

**Cause:** Loop is blocked for > 10 seconds.

**Debug approach:**

```cpp
void loop() {
    Serial.println("Loop start");

    server.handleClient();
    Serial.println("After server.handleClient()");

    readTemperatures();
    Serial.println("After readTemperatures()");

    yield();  // Feed watchdog
}
```

**Common culprits:**
- SD card operations without timeout
- Blocking network calls
- Large file transfers
- Missing `yield()` in long loops

### Memory Leak Detection

```cpp
void loop() {
    static unsigned long lastMemCheck = 0;

    if (millis() - lastMemCheck > 10000) {
        Serial.printf("[MEM] Free heap: %d bytes\n", ESP.getFreeHeap());
        lastMemCheck = millis();
    }
}
```

Watch for decreasing free heap over time. If heap drops continuously:
- Check for `new` without `delete`
- Check for `String` concatenation in loops
- Check for growing vectors without clearing

---

## Gotchas & Known Issues

### 1. SD Card Initialization Timing

**Issue:** SD card fails to initialize if initialized too early.

**Workaround:** Initialize SD after display initialization:

```cpp
void setup() {
    initDisplay();  // Must be first
    delay(100);     // Give display time to settle
    initSDCard();   // Then SD card
}
```

### 2. LovyanGFX SPI Bus Conflicts

**Issue:** Display SPI conflicts with SD card SPI if both use same bus.

**Solution:** CYD uses separate SPI buses:
- Display: HSPI (SPI1)
- SD Card: VSPI (SPI2)

Pin definitions in `pins.h` must match this configuration.

### 3. WebSocket Disconnect/Reconnect Loop

**Issue:** WebSocket disconnects and reconnects rapidly.

**Causes:**
- FluidNC IP unreachable
- Network congestion
- Firewall blocking connection

**Debug:**

```cpp
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_DISCONNECTED) {
        Serial.println("[WS] Disconnected - check FluidNC IP");
    }
}
```

**Solution:** Verify FluidNC IP in `/settings` page, check network connectivity.

### 4. Temperature Readings Show -127°C

**Issue:** DS18B20 returns -127°C (0x00) when not found or wiring issue.

**Causes:**
- Sensor not connected
- Wrong GPIO pin
- Missing 4.7kΩ pull-up resistor on data line
- Sensor power issue

**Debug:**

```cpp
void readTemperatures() {
    sensors.requestTemperatures();

    for (int i = 0; i < 4; i++) {
        float temp = sensors.getTempCByIndex(i);
        if (temp == -127.0 || temp == 85.0) {
            Serial.printf("Sensor %d: ERROR (reading %.1f)\n", i, temp);
        }
    }
}
```

### 5. JSON Parsing Fails Silently

**Issue:** ArduinoJson parsing returns empty values without error.

**Cause:** Document size too small for JSON content.

**Solution:** Increase `JsonDocument` size:

```cpp
// Before:
JsonDocument doc;  // Default size

// After:
JsonDocument doc;  // Auto-sizes in v7
// Or explicit:
StaticJsonDocument<2048> doc;  // For smaller, fixed-size docs
```

**Debug:**

```cpp
DeserializationError error = deserializeJson(doc, file);
if (error) {
    Serial.print("JSON error: ");
    Serial.println(error.c_str());
}
```

### 6. Config Not Persisting After Reboot

**Issue:** Settings save but revert to defaults after reboot.

**Causes:**
- SD card removed
- File write failed
- Using SPIFFS but uploaded new firmware (erases SPIFFS)

**Debug:**

```cpp
void saveConfig() {
    File file = storage.openFile("/config.json", FILE_WRITE);
    if (!file) {
        Serial.println("ERROR: Failed to open config.json for writing");
        return;
    }

    serializeJson(doc, file);
    file.close();

    // Verify write
    File verify = storage.openFile("/config.json", FILE_READ);
    if (verify && verify.size() > 0) {
        Serial.println("Config saved successfully");
    } else {
        Serial.println("ERROR: Config file empty after save");
    }
}
```

### 7. Display Flickers or Shows Artifacts

**Issue:** Display flickers during updates or shows visual artifacts.

**Causes:**
- Redrawing entire screen on every update
- SPI bus speed too high
- Power supply voltage sag

**Solution:** Use partial updates:

```cpp
// Bad - full screen redraw
void updateDisplay() {
    tft.fillScreen(TFT_BLACK);  // Clears entire screen
    drawMonitorMode();
}

// Good - update only changed values
void updateDisplay() {
    // Clear only the area being updated
    tft.fillRect(100, 50, 120, 30, TFT_BLACK);
    tft.setCursor(100, 50);
    tft.printf("X: %.3f", posX);
}
```

### 8. WiFi Credentials Lost

**Issue:** Device enters AP mode after reboot despite previously working WiFi.

**Cause:** WiFiManager stores credentials in NVS (Preferences), which can be erased by certain operations.

**Solution:** Use Preferences API to check:

```cpp
prefs.begin("wifi", true);
if (!prefs.isKey("ssid")) {
    Serial.println("No WiFi credentials stored");
}
prefs.end();
```

### 9. Out of Memory During JSON Parsing

**Issue:** `deserializeJson()` fails with `NoMemory` error for large files.

**Solution:** Use streaming parser or increase heap:

```cpp
// For large JSON files
JsonDocument doc;
DeserializationError error = deserializeJson(doc, file);

if (error == DeserializationError::NoMemory) {
    Serial.println("Not enough memory for JSON");
    // Consider: reduce JSON size, stream parsing, or increase PSRAM
}
```

### 10. Tachometer Reads Zero RPM

**Issue:** Fan tachometer always shows 0 RPM despite fan running.

**Causes:**
- GPIO not configured for INPUT
- Missing interrupt attachment
- Fan doesn't have tachometer output (3-wire vs 4-wire fan)

**Debug:**

```cpp
void setup() {
    pinMode(FAN_TACH_PIN, INPUT_PULLUP);  // Must be INPUT_PULLUP
    attachInterrupt(digitalPinToInterrupt(FAN_TACH_PIN), tachISR, FALLING);

    Serial.printf("Tachometer on GPIO %d\n", FAN_TACH_PIN);
}

void loop() {
    Serial.printf("Tach pulses: %d\n", tachCounter);  // Should increase
}
```

---

## Best Practices for AI Assistants

### When Analyzing This Codebase

**DO:**
- ✅ Read `config.h` first to understand data structures
- ✅ Check `pins.h` before modifying GPIO assignments
- ✅ Review `SENSOR_MANAGEMENT_IMPLEMENTATION.md` before touching sensor code
- ✅ Test code changes with both SD card present and absent
- ✅ Consider memory constraints (ESP32 has ~320KB usable RAM)
- ✅ Maintain non-blocking patterns in `loop()`
- ✅ Add error handling for all file operations
- ✅ Use `extern` for cross-module variable access
- ✅ Follow existing naming conventions

**DON'T:**
- ❌ Add `delay()` calls in main loop
- ❌ Allocate large buffers on stack
- ❌ Use `String` class for frequently-updated values
- ❌ Hardcode IP addresses or credentials
- ❌ Remove watchdog yields without justification
- ❌ Change GPIO pins without verifying hardware compatibility
- ❌ Ignore return values from SD/SPIFFS functions
- ❌ Create new global variables without clear need

### When Making Code Changes

**1. Understand the impact:**
```cpp
// Before changing this:
extern float temperatures[4];

// Ask: What code depends on this array size?
// Search codebase for "temperatures[" usage
// Check if JSON layouts reference temp0-temp3
```

**2. Preserve backward compatibility:**
```cpp
// When adding new config fields:
cfg.new_field = doc["new_field"] | DEFAULT_VALUE;
//                                 ↑ Provide default for old configs
```

**3. Test fallback paths:**
```cpp
// If you change SD card code, test with:
// 1. SD card present
// 2. SD card removed
// 3. SD card with missing files
// 4. SD card with corrupted files
```

**4. Maintain code style:**
```cpp
// Match existing style:
void newFunction() {  // Not: void new_function()
    int localVar = 0;  // Not: int local_var = 0
}
```

### When Explaining Code to Users

**Be specific about file locations:**

```
❌ "The temperature is read in the sensor module"
✅ "The temperature is read in readTemperatures() at src/sensors/sensors.cpp:45"
```

**Reference configuration:**

```
❌ "Change the threshold"
✅ "Update cfg.temp_threshold_high in the web settings page or modify the default in src/config/config.cpp:23"
```

**Explain dependencies:**

```
❌ "Add the DHT library"
✅ "Add 'adafruit/DHT sensor library@^1.4.4' to the lib_deps section in platformio.ini"
```

### When Debugging Issues

**1. Gather information systematically:**

```markdown
Please provide:
- Serial monitor output during boot
- Current FluidDash version (shown on splash screen)
- SD card status (inserted? detected?)
- WiFi connection status (IP address?)
- Error messages in serial output
```

**2. Check common failure points:**

```cpp
// 1. Is hardware initialized?
if (!rtcAvailable) {
    Serial.println("RTC not detected - check I2C connections");
}

// 2. Are files accessible?
if (!storage.exists("/config.json")) {
    Serial.println("Config file missing - using defaults");
}

// 3. Is network connected?
if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
}

// 4. Is FluidNC reachable?
if (!fluidncConnected) {
    Serial.printf("Cannot connect to FluidNC at %s:%d\n",
                  cfg.fluidnc_ip, cfg.fluidnc_port);
}
```

**3. Suggest incremental tests:**

```markdown
Let's test step-by-step:

1. First, verify display works:
   - Upload firmware
   - Check for splash screen
   - Try cycling modes with button

2. Then, test sensors:
   - Check serial output for temperature readings
   - Verify readings are not -127°C

3. Finally, test networking:
   - Verify WiFi connection (check for IP address)
   - Access web interface at http://<device-ip>/
   - Test WebSocket connection to FluidNC
```

### When Proposing Architectural Changes

**Consider:**
- **Memory impact:** Will this increase RAM usage significantly?
- **Compatibility:** Will existing configs/layouts still work?
- **Complexity:** Does this add significant complexity for maintainability?
- **Hardware constraints:** Does ESP32 support this feature?

**Document tradeoffs:**

```markdown
## Proposed Change: Add MQTT Support

### Benefits:
- Enables remote monitoring
- Integrates with home automation
- Provides data logging capability

### Drawbacks:
- Adds ~30KB to firmware size
- Increases RAM usage by ~8KB
- Requires additional configuration
- May conflict with WebSocket resource usage

### Implementation Considerations:
- Use WiFiClient instead of WiFiClientSecure (saves 20KB)
- Limit MQTT message queue to 5 messages (saves RAM)
- Make MQTT optional (disable via config)
```

### When Creating Documentation

**Use concrete examples:**

```markdown
❌ "Configure the sensor settings"

✅ "Configure sensor settings in /admin page:
   1. Click 'Calibration' tab
   2. Enter offset for each sensor (-5.0 to +5.0)
   3. Click 'Save Calibration'
   4. Restart device for changes to take effect"
```

**Include expected outcomes:**

```markdown
After running this code, you should see:

Serial Output:
```
[SENSOR] Discovering DS18B20 sensors...
[SENSOR] Found 4 sensors
[SENSOR] UID 0: 28FF641E8C160450
[SENSOR] UID 1: 28AA32FC5D160468
```

If you see "Found 0 sensors", check OneWire pin connection.
```

---

## Appendix

### Pin Reference (ESP32-2432S028)

**Complete GPIO assignment from `src/config/pins.h`:**

| GPIO | Function | Description |
|------|----------|-------------|
| 0 | MODE_BUTTON_PIN | Boot button (mode switch) |
| 4 | FAN_PWM_PIN | Fan PWM control |
| 5 | SD_CS_PIN | SD card chip select |
| 14 | TFT_DC | Display data/command |
| 15 | TFT_CS | Display chip select |
| 16 | RGB_LED_G | RGB LED - Green |
| 17 | RGB_LED_B | RGB LED - Blue |
| 18 | TFT_SCK | Display SPI clock |
| 21 | TEMP_SENSOR_PIN | OneWire data (DS18B20) |
| 22 | RGB_LED_R | RGB LED - Red |
| 23 | TFT_MOSI | Display SPI MOSI |
| 25 | I2C_SCL | RTC I2C clock |
| 27 | TFT_BL | Display backlight |
| 32 | I2C_SDA | RTC I2C data |
| 33 | TOUCH_CS | Touchscreen chip select |
| 34 | PSU_VOLTAGE_PIN | PSU voltage ADC |
| 35 | FAN_TACH_PIN | Fan tachometer |

### Color Reference (RGB565)

Common colors used in FluidDash (16-bit RGB565 format):

```cpp
#define TFT_BLACK       0x0000  // Black
#define TFT_WHITE       0xFFFF  // White
#define TFT_RED         0xF800  // Red
#define TFT_GREEN       0x07E0  // Green
#define TFT_BLUE        0x001F  // Blue
#define TFT_CYAN        0x07FF  // Cyan
#define TFT_MAGENTA     0xF81F  // Magenta
#define TFT_YELLOW      0xFFE0  // Yellow
#define TFT_ORANGE      0xFD20  // Orange
#define TFT_NAVY        0x000F  // Navy blue
#define TFT_DARKGREEN   0x03E0  // Dark green
#define TFT_DARKCYAN    0x03EF  // Dark cyan
```

**Convert RGB to RGB565:**

```python
# Python helper
def rgb_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

# Example: Orange (255, 165, 0)
orange = rgb_to_rgb565(255, 165, 0)  # = 0xFD20
```

### Library Versions (Verified Compatible)

```ini
[env:esp32dev]
lib_deps =
    adafruit/RTClib@^2.1.4
    lovyan03/LovyanGFX@^1.1.16
    https://github.com/tzapu/WiFiManager.git
    links2004/WebSockets@2.7.1
    bblanchon/ArduinoJson@^7.2.0
    paulstoffregen/OneWire@^2.3.8
    milesburton/DallasTemperature@^3.11.0
```

### Useful Resources

- **PlatformIO Docs:** https://docs.platformio.org/
- **ESP32 Arduino Core:** https://github.com/espressif/arduino-esp32
- **LovyanGFX:** https://github.com/lovyan03/LovyanGFX
- **FluidNC:** https://github.com/bdring/FluidNC
- **ArduinoJson:** https://arduinojson.org/
- **CYD Hardware:** https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display

---

## Changelog

**2025-01-20:** Documentation consolidation and refactoring completion
- All core refactoring phases (1, 2, 4, 5, 7) marked complete
- Historical planning docs archived to `docs/archive/`
- CLAUDE.md updated as single source of truth
- Production-ready codebase with professional architecture

**2025-01-18:** Phases 4, 5, 7 and Driver Assignment complete
- Phase 7: Temperature sensor naming interface with NVS persistence
- Driver position assignment system (touch detection for sensor mapping)
- Phase 4: Web server optimization
- Phase 5: Code cleanup and organization
- FluidNC configuration tested and working

**2025-01-17:** Phases 1, 2, and critical fixes complete
- Phase 1: Storage system initialized, HTML files loading from filesystem
- Phase 2: JSON screen rendering disabled (hard-coded screens)
- Critical boot loop fix (watchdog timeout resolved)
- WiFi and FluidNC made optional (standalone mode works)
- Sensor management system implemented

**2025-11-14:** Initial CLAUDE.md creation
- Comprehensive codebase documentation
- Development workflows and conventions
- Architecture and design patterns
- Common tasks and debugging guides
- Best practices for AI assistants

---

## License & Contributing

This is a personal project. When contributing code:

1. Follow existing code conventions
2. Test on hardware when possible
3. Document new features in comments
4. Update this CLAUDE.md for architectural changes

---

**End of CLAUDE.md**


---

## Recent Updates (2025-01-21)

### Touchscreen Support + Data Logging Complete ✅

**Status:** v0.3.000 - All three enhancement phases complete and tested on hardware

Three major feature additions have been successfully implemented, tested, and merged into `main`:

### Phase 1: Layout Refactoring ✅

**Goal:** Make screen layouts easier to edit by centralizing hard-coded positions, font sizes, and spacing values.

**Implementation:**
- Created `src/display/ui_layout.h` with 150+ centralized constants
- Organized into namespaces: `MonitorLayout`, `AlignmentLayout`, `GraphLayout`, `NetworkLayout`
- Refactored all 4 UI files (`ui_monitor.cpp`, `ui_alignment.cpp`, `ui_graph.cpp`, `ui_network.cpp`)
- Changed from scattered magic numbers to centralized, easily editable values

**Example:**
```cpp
// Before: Magic numbers scattered throughout code
gfx.setCursor(10, 50);
gfx.setTextSize(2);

// After: Centralized constants
gfx.setCursor(MonitorLayout::TEMP_LABEL_X, MonitorLayout::TEMP_START_Y);
gfx.setTextSize(MonitorLayout::TEMP_LABEL_FONT_SIZE);
```

**Benefits:**
- Easy editing: Change layout by modifying constants in one file
- Consistency: All screens use organized constant sets
- Maintainability: Clear intent and easy to understand

### Phase 2: Touchscreen Support ✅

**Goal:** Enable XPT2046 resistive touchscreen for basic navigation without requiring the physical button.

**Implementation:**
- Initialized XPT2046 touch controller in `display.h/cpp`
- Proper calibration values (X: 300-3900, Y: 62000-65500 raw ADC)
- Created `src/input/touch_handler.h/cpp` with touch zone detection
- **Safety mechanism:** 5-second hold requirement for WiFi setup (prevents accidental activation)

**Touch Zones:**
- **Bottom of screen (Y > 280):** Tap to cycle display modes (instant, 300ms debounce)
- **Top of screen (Y < 25):** Hold for 5 seconds to enter WiFi setup mode
  - Visual progress bar (0-100%) with cyan fill and percentage display
  - Automatically cancels if touch is released or moved
- **Middle zone:** No action (safe zone)

**Features:**
- Debouncing: 300ms for footer tap to prevent double-activation
- Progress bar: Visual feedback during 5-second hold
- Cancellation: Releases hold if finger moves or lifts
- Redundancy: Physical button still works (backup input method)

**Serial Debug Output:**
```
[TOUCH] Header hold started - hold for 5s to enter WiFi setup
[TOUCH] Header hold complete - entering setup mode
[TOUCH] Footer zone tapped - cycling mode
```

### Phase 3: Data Logging ✅

**Goal:** Implement CSV data logging to SD card for historical sensor tracking.

**Implementation:**
- Created `src/logging/data_logger.h/cpp` with DataLogger class
- RTC-based timestamps and filenames
- Automatic file rotation at 10MB
- Web API endpoints for remote control

**CSV Format:**
```csv
Timestamp,TempX,TempYL,TempYR,TempZ,PSU_Voltage,Fan_RPM,Fan_Speed,Machine_State,Pos_X,Pos_Y,Pos_Z
2025-01-21 15:12:00,26.3,26.4,26.2,26.4,13.52,0,30,IDLE,0,0,0
```

**Features:**
- **Filename:** `/logs/fluiddash_YYYYMMDD.csv` (daily files)
- **Interval:** 10 seconds default (configurable 1s-10min)
- **Timestamps:** RTC-based (`2025-01-21 15:12:00`) or uptime fallback
- **Rotation:** Automatic 10MB file size limit
- **Headers:** Automatically written for new files
- **Disabled by default** to preserve SD card lifespan

**Web API Endpoints:**
```
POST /api/logs/enable     - Enable/disable logging + set interval
GET  /api/logs/status     - Current logging status
GET  /api/logs/list       - List all CSV log files
GET  /api/logs/download   - Download specific log file
DELETE /api/logs/clear    - Delete all log files
```

**Example Usage:**
```javascript
// Enable logging with 10-second interval
fetch('/api/logs/enable', {
  method: 'POST',
  headers: {'Content-Type': 'application/json'},
  body: JSON.stringify({enabled: true, interval: 10000})
});

// Download log file
window.open('/api/logs/download?file=fluiddash_20250121.csv');
```

### Hardware Testing Confirmation

All three phases tested on ESP32-2432S028 hardware:
- ✅ Touchscreen calibration correct (zones work as expected)
- ✅ 5-second hold safety mechanism prevents accidental WiFi setup
- ✅ Progress bar displays correctly during hold
- ✅ CSV logging creates proper files with headers and RTC timestamps
- ✅ Physical button still works (redundancy maintained)
- ✅ No performance degradation or UI lag
- ✅ Non-blocking operation throughout

### Files Added/Modified

**New Files (6):**
- `src/display/ui_layout.h` - 150+ centralized layout constants
- `src/input/touch_handler.h` - Touch input declarations
- `src/input/touch_handler.cpp` - Touch zone detection and progress bar
- `src/logging/data_logger.h` - DataLogger class
- `src/logging/data_logger.cpp` - CSV logging implementation

**Modified Files (5):**
- `src/display/display.cpp` - XPT2046 touch calibration
- `src/display/ui_*.cpp` - Layout constant refactoring (4 files)
- `src/main.cpp` - Touch handler integration, logger initialization
- `src/web/web_handlers.h` - Logging API declarations
- `src/web/web_handlers.cpp` - Logging API endpoints

**Total:** 10 commits, ~15 files changed

---

## Recent Updates (2025-01-22)

### Production Enhancements Complete ✅

**Status:** v0.3.100 - All user-requested features implemented and tested

This session focused on production quality improvements, best practices implementation, and user experience enhancements.

### Key Features Added

#### 1. Temperature Unit Selection (Celsius/Fahrenheit)

**Implementation:**
- User-selectable temperature unit in Settings page
- Real-time conversion in web interface with JavaScript
- LCD display respects user preference
- All temperatures stored internally as Celsius
- Auto-conversion for display throughout system

**Files Modified:**
- `data/web/settings.html` - Added unit selector with live conversion
- `data/web/main.html` - Temperature display with correct unit
- `data/web/admin.html` - Current readings in user's unit
- `src/web/web_handlers.cpp` - Status JSON includes temp_unit field
- `src/display/ui_monitor.cpp` - convertTemp() helper function
- `src/display/ui_alignment.cpp` - Temperature display conversion

**User Experience:**
- Change dropdown: values instantly convert (30°C ↔ 86°F)
- Save: stores as Celsius internally
- Reload: displays in saved preference
- LCD and web always match

#### 2. Browser Cache Management

**Issue:** Settings page cached by browser, showed stale values when navigating back from main page.

**Solution:** Added no-cache headers to settings and admin pages
```cpp
server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
server.sendHeader("Pragma", "no-cache");
server.sendHeader("Expires", "0");
```

**Result:** Pages always show current configuration without manual refresh

#### 3. FluidNC Connection Reliability (ARP Cache Fix)

**Issue:** FluidNC running for days, FluidDash wouldn't connect after reboot until user opened browser to FluidNC.

**Root Cause:** Network switch ARP cache didn't have FluidNC's MAC address for idle device.

**Solution:** HTTP pre-connection to populate ARP cache before WebSocket attempt
```cpp
// Send HTTP GET to FluidNC before WebSocket
WiFiClient client;
if (client.connect(cfg.fluidnc_ip, 80)) {
    // ARP cache now populated, WebSocket will succeed
}
```

**Result:** Reliable connection on first attempt without manual intervention

#### 4. Factory Reset to Defaults

**Motivation:** Following embedded systems best practices for field deployment and troubleshooting.

**Implementation:**
- `resetToDefaults()` function clears entire NVS namespace
- Web UI in admin page with prominent danger styling
- Double confirmation dialogs prevent accidental activation
- Lists exactly what gets deleted
- Auto-restart after reset

**User Interface:**
- Red border and danger colors
- Two scary confirmation dialogs
- Clear consequences listed
- Restarts automatically to load defaults

**Use Cases:**
- User misconfiguration recovery
- Field support ("try factory reset first")
- Device redeployment
- NVS corruption recovery

#### 5. Storage Mode (5th Display Mode)

**Added:** New LCD screen showing storage and logging status

**Displays:**
- SD card availability and free space
- SPIFFS status and free space
- Data logging enabled/disabled status
- Current log file and size
- Total log file count

**Benefits:**
- At-a-glance storage status without web access
- Useful for standalone operation
- Know when SD card needs attention

#### 6. SD Card Error Handling

**Issue:** SD card write failure caused overnight system hang at 3:29 PM.

**Solution:** Comprehensive error handling with graceful degradation
- Failure counter tracks consecutive SD errors
- Auto-disable logging after 5 failures
- System continues operating even if SD fails
- Detailed serial error messages for diagnostics

**Result:** SD issues can't hang the entire system

#### 7. Configuration Cleanup

**Removed:** Redundant `initDefaultConfig()` function
- Function set defaults that were immediately overwritten by `loadConfig()`
- Caused confusion with inconsistent defaults between functions
- Reduced code by 39 lines

**Benefits:**
- Single source of truth for defaults (in `loadConfig()` calls)
- No ambiguity about which defaults are used
- Cleaner, more maintainable code

### Configuration Defaults Updated

Changed defaults to better user experience:

| Setting | Old Default | New Default | Reason |
|---------|-------------|-------------|---------|
| `use_fahrenheit` | false | **true** | US market preference |
| `enable_logging` | false | **true** | Logging ready out of box |

### Debug Features Added

**Serial Output:**
- `[Settings] use_fahrenheit = X` - Shows current temp unit setting
- `[API Save] use_fahrenheit received: X, set to: X` - Confirms save
- `[FluidNC] ARP cache populated` - Connection diagnostics
- `[LOGGER] SD failures tracked` - Storage diagnostics

### Files Modified This Session

**Configuration:**
- `src/config/config.h` - Added resetToDefaults() declaration
- `src/config/config.cpp` - Removed initDefaultConfig(), added resetToDefaults()

**Web Interface:**
- `data/web/settings.html` - Temperature unit selector with live conversion
- `data/web/main.html` - Temperature display with unit from API
- `data/web/admin.html` - Factory reset section, temp unit display
- `src/web/web_handlers.h` - Added handleAPIResetToDefaults()
- `src/web/web_handlers.cpp` - No-cache headers, temp conversion, reset API

**Display:**
- `src/display/ui_layout.h` - Added StorageLayout namespace
- `src/display/ui_monitor.cpp` - Added convertTemp() helper
- `src/display/ui_alignment.cpp` - Temperature unit conversion
- `src/display/ui_storage.cpp` - **NEW** Storage mode implementation
- `src/display/ui_modes.cpp` - Added Storage mode to switch statements
- `src/display/ui_modes.h` - Storage mode declarations

**Network:**
- `src/network/network.cpp` - HTTP pre-connection for ARP cache

**Logging:**
- `src/logging/data_logger.cpp` - SD error handling with auto-disable

**Input:**
- `src/input/touch_handler.cpp` - Updated mode cycle to 5 modes

**Total:** 18 files modified, 8 commits

### Testing Status

All features tested on hardware:
- ✅ Temperature unit toggle works in web and LCD
- ✅ Settings page no longer caches (shows current values)
- ✅ FluidNC connects reliably after reboot
- ✅ Factory reset clears all settings correctly
- ✅ Storage mode displays accurate information
- ✅ SD error handling prevents system hangs
- ✅ No watchdog resets or performance issues

### Production Readiness Checklist

✅ **Code Quality** - Professional embedded systems architecture
✅ **Error Handling** - Graceful degradation throughout
✅ **User Experience** - Intuitive interface with smart defaults
✅ **Best Practices** - Factory reset, proper caching, error recovery
✅ **Documentation** - Comprehensive CLAUDE.md, inline comments
✅ **Testing** - Hardware validated, overnight stability testing
✅ **Reliability** - Non-blocking operations, watchdog management
✅ **Maintainability** - Modular architecture, clear separation of concerns

**Overall Status:** Production-ready for field deployment

---

## Recent Updates (2025-11-19)

### Architecture Refactoring Complete ✅

**Status:** Professional modular architecture achieved (100% complete)

The codebase has undergone comprehensive refactoring to achieve production-ready modular architecture. All recommendations from the Claude Opus Code Review have been successfully implemented and tested.

### Code Metrics After Refactoring

| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| **main.cpp size** | 1,192 lines | 260 lines | 78% reduction |
| **Web handlers** | In main.cpp | Extracted (813 lines) | 100% modular |
| **Global variables** | Scattered | Structured | Professional |
| **WebSocket code** | 25 lines in main | 1 function call | 96% cleaner |
| **Module organization** | Partial | Complete | Production-ready |

### New Architecture Overview

#### Module Structure

```
src/
├── main.cpp                  # Core orchestration (260 lines)
├── config/
│   ├── config.h/cpp          # Configuration management
│   └── pins.h                # Hardware pin definitions
├── display/
│   ├── display.h/cpp         # Display + touchscreen initialization
│   ├── ui_layout.h           # ⭐ Centralized layout constants (150+ values)
│   ├── ui_modes.h            # UI mode declarations
│   ├── ui_monitor.cpp        # Monitor screen
│   ├── ui_alignment.cpp      # Alignment screen
│   ├── ui_graph.cpp          # Graph screen
│   ├── ui_network.cpp        # Network status screen
│   └── ui_common.cpp         # Shared UI functions
├── input/
│   ├── touch_handler.h       # ⭐ NEW: Touchscreen input handling
│   └── touch_handler.cpp     # ⭐ NEW: Touch zones, hold detection, progress bar
├── logging/
│   ├── data_logger.h         # ⭐ NEW: CSV data logger
│   └── data_logger.cpp       # ⭐ NEW: RTC timestamps, file rotation
├── network/
│   ├── network.h/cpp         # WiFi & WebSocket (100% complete)
│   └── [handleWebSocketLoop] # WebSocket loop extracted here
├── sensors/
│   └── sensors.h/cpp         # Temperature, PSU, fan control
├── state/
│   ├── global_state.h        # Structured state definitions
│   └── global_state.cpp      # State initialization
├── utils/
│   └── utils.h/cpp           # Utility functions
└── web/
    ├── web_handlers.h        # Web handler declarations
    ├── web_handlers.cpp      # All web handlers (813 lines + logging APIs)
    └── web_utils.h           # HTTP utilities & ETag caching
```

#### State Management (New in v0.2)

All global variables are now organized into structured types in `src/state/global_state.h`:

```cpp
// Sensor state
extern SensorState sensors;
  // .temperatures[4], .peakTemps[4], .psuVoltage
  // .fanSpeed, .fanRPM, .tachCounter
  // .adcSamples, .adcSampleIndex, .adcReady

// Temperature history
extern HistoryState history;
  // .tempHistory, .historySize, .historyIndex

// FluidNC state
extern FluidNCState fluidnc;
  // .machineState, .connected, .connectionAttempted
  // .posX/Y/Z/A, .wposX/Y/Z/A, .wcoX/Y/Z/A
  // .feedRate, .spindleRPM, overrides, timing

// Network state
extern NetworkState network;
  // .inAPMode, .webServerStarted, .rtcAvailable

// Timing state
extern TimingState timing;
  // .lastTachRead, .lastDisplayUpdate, .lastHistoryUpdate
  // .lastStatusRequest, .sessionStartTime, .buttonPressStart
  // .bootCompleteTime, .buttonPressed
```

**Benefits:**
- Clear ownership and organization
- Easy to understand data flow
- Consistent access patterns throughout codebase
- Proper extern declarations eliminate linker issues
- Professional embedded systems architecture

#### Web Handler Extraction (New in v0.2)

All web-related code has been extracted to dedicated module:

**File:** `src/web/web_handlers.cpp` (813 lines)

**Contents:**
- All HTTP request handlers (`handleRoot`, `handleSettings`, etc.)
- All API endpoints (`handleAPIConfig`, `handleAPIStatus`, etc.)
- All HTML generation functions
- Web server setup and configuration
- Complete isolation from main.cpp

**Benefits:**
- main.cpp focuses on core orchestration only
- Web features can be tested independently
- Easy to add new web endpoints
- Clear separation between hardware and UI

#### Network Module Completion (New in v0.2)

WebSocket handling logic fully consolidated:

**Function:** `handleWebSocketLoop()` in `src/network/network.cpp`

**Responsibilities:**
- WebSocket event processing
- FluidNC status polling
- Connection management
- Debug output (when enabled)

**Integration in main.cpp:**
```cpp
void loop() {
    server.handleClient();
    handleButton();
    sampleSensorsNonBlocking();
    // ... sensor processing ...
    handleWebSocketLoop();  // Single call, all logic in network module
    updateDisplay();
    yield();
}
```

### Code Quality Status

✅ **Module Organization:** Professional - each module has single clear purpose  
✅ **State Management:** Structured - all globals properly organized  
✅ **Separation of Concerns:** Complete - hardware/network/display/web isolated  
✅ **Maintainability:** Excellent - easy to locate and modify code  
✅ **Testability:** High - modules can be tested independently  
✅ **Documentation:** Comprehensive - well-commented with clear intent  
✅ **Error Handling:** Robust - proper validation and returns  
✅ **Non-blocking:** Consistent - millis() timing throughout  

**Overall Assessment:** Production-ready embedded systems architecture

### Testing Confirmation (2025-11-19)

All refactoring changes have been:
- ✅ Compiled successfully (no errors or warnings)
- ✅ Uploaded to ESP32-2432S028 hardware
- ✅ Tested with basic functionality checks
- ✅ Verified all display modes working
- ✅ Confirmed sensor readings accurate
- ✅ Validated web server accessible
- ✅ Checked WebSocket connection functioning
- ✅ No watchdog timer resets observed

### Development Tools Used

- **Primary IDE:** VS Code with PlatformIO extension
- **AI Assistant:** Claude Code (VS Code extension)
- **Refactoring Support:** Claude Opus via browser (with Desktop Commander MCP)
- **Version Control:** Git
- **Hardware:** ESP32-2432S028 (CYD 3.5")

### For Future AI Assistants

When working on this codebase:

1. **State Access:** Use structured globals (e.g., `sensors.temperatures[0]`, `fluidnc.machineState`)
2. **Web Changes:** Modify `src/web/web_handlers.cpp`, not main.cpp
3. **Network Changes:** Modify `src/network/network.cpp`, including WebSocket logic
4. **New Features:** Consider which module owns the functionality before coding
5. **main.cpp:** Should only contain orchestration logic, not implementation details

### Reference Documents

- **PROGRESS_LOG.md** - Detailed session-by-session development history
- **Claude_Opus_Code_Review_FluidDash-v02.md** - Comprehensive code quality assessment
- **Claude_Code_Instructions_WebSocket_Extraction.md** - Final refactoring instructions
- **TESTING_CHECKLIST.md** - Hardware and software testing procedures
- **PHASE_6_TESTING_PLAN.md** - Long-term stability testing plan

---

**Architecture Version:** 2.0  
**Code Quality Grade:** A+ (Professional)  
**Refactoring Status:** Complete ✅  
**Production Status:** Ready ✅

