# CLAUDE.md - AI Assistant Development Guide

**Project:** FluidDash v02 - ESP32 CYD Edition
**Last Updated:** 2025-11-14
**Version:** 0.2.001

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

**FluidDash v02** is an embedded IoT firmware for the **ESP32-2432S028 (CYD - Cheap Yellow Display)** that provides real-time monitoring and control for CNC machining environments. It acts as a bedside dashboard displaying machine status from FluidNC (open-source CNC firmware) while simultaneously tracking environmental conditions via temperature sensors, PSU voltage, and cooling fan performance.

### Key Capabilities

- **Real-time CNC monitoring** via WebSocket connection to FluidNC
- **480×320 IPS LCD display** with multiple viewing modes (Monitor, Graph, Alignment, Network)
- **Temperature monitoring** using DS18B20 OneWire sensors (supports 4+)
- **Web-based configuration** interface (HTML/CSS/JavaScript)
- **JSON-defined screen layouts** for customization without recompilation
- **Dual storage system** (SD card primary, LittleFS fallback)
- **Fan control** with temperature-based PWM and tachometer feedback
- **PSU voltage monitoring** with alert thresholds

### Hardware Platform

**Target Device:** ESP32-2432S028 (CYD)
- **MCU:** Dual-core ESP32 @ 240MHz (4MB flash)
- **Display:** 480×320 IPS LCD (ST7796 controller, SPI interface)
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
- ✅ Check if SD card layouts exist in `/data/screens/*.json` before modifying display code
- ✅ Review `Config` struct in `config.h` before adding configuration options
- ✅ Use `extern` declarations for globals accessed across modules
- ✅ Test with both SD card present and absent (dual-storage fallback)
- ✅ Verify changes don't exceed available RAM (~320KB usable)
- ✅ Check `SENSOR_MANAGEMENT_IMPLEMENTATION.md` for planned sensor system changes

**NEVER:**
- ❌ Use blocking delays in `loop()` - this is a real-time system
- ❌ Allocate large buffers on stack (use heap with `new`/`malloc`)
- ❌ Hardcode IP addresses or credentials
- ❌ Remove watchdog timer yields (causes reboot)
- ❌ Use `String` class for large or frequently-changed strings (use `char[]`)
- ❌ Ignore return values from SD/SPIFFS operations

---

## Repository Structure

```
FluidDash-v02/
├── platformio.ini                 # PlatformIO build configuration
├── CLAUDE.md                      # This file - AI assistant guide
├── SENSOR_MANAGEMENT_IMPLEMENTATION.md  # Sensor system implementation plan
├── html_to_spiffs.md              # Guide for embedding HTML in SPIFFS
│
├── data/                          # Filesystem data (uploaded to SPIFFS/SD)
│   ├── web/                       # Web interface HTML files
│   │   ├── main.html              # Main dashboard
│   │   ├── settings.html          # User configuration
│   │   ├── admin.html             # Calibration interface
│   │   └── wifi_config.html       # WiFi setup page
│   ├── screens/                   # JSON screen layout definitions
│   │   ├── screen_0.json          # Monitor mode layout
│   │   ├── monitor.json           # Alternative monitor layout
│   │   ├── alignment.json         # Alignment/calibration screen
│   │   ├── graph.json             # Temperature graphing screen
│   │   └── network.json           # Network status screen
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
**Development Branches:** `claude/*` (AI-assisted development)
**Feature Branches:** `feature/*` (manual development)

**Current Working Branch:** `claude/claude-md-mhzdss8idgpqrjlf-01DLKhNtvYbXckbVfLBRStQ5`

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

#### 4. JSON-Based Configuration

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

// Sensor configuration (planned - see SENSOR_MANAGEMENT_IMPLEMENTATION.md)
void loadSensorConfig();              // Load UID-to-name mapping
void saveSensorConfig();              // Save sensor configuration
String detectTouchedSensor(unsigned long timeout);  // Touch detection
```

**Data Structures:**
```cpp
struct SensorMapping {
    uint8_t uid[8];           // DS18B20 unique ROM address
    char friendlyName[32];    // "X-Axis Motor"
    char alias[16];           // "temp0"
    bool enabled;
    char notes[64];
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

**JSON Layout System:**

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
| `/api/config` | GET/POST | Get/set configuration |
| `/api/status` | GET | System status (JSON) |
| `/api/rtc` | GET/POST | RTC time management |
| `/api/reload-screens` | POST | Reload JSON layouts |
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
