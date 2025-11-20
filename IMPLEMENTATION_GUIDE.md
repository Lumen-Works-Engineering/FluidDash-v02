# FluidDash UI Enhancement Implementation Guide

**Branch:** `feature/touchscreen-and-layout`
**Session Start:** 2025-11-20
**Goal:** Implement layout refactoring, touchscreen support, and data logging

---

## 🎯 Implementation Overview

### Phase 1: Layout Refactoring (PRIORITY 1)
**Goal:** Extract all hard-coded layout values to centralized configuration file

**Why:** Makes editing screen layouts 10x easier - change font sizes/positions in one place instead of hunting through 200+ lines of code per screen.

**Files to Create:**
- `src/display/ui_layout.h` - All layout constants organized by screen mode

**Files to Modify:**
- `src/display/ui_monitor.cpp` - Refactor to use MonitorLayout constants
- `src/display/ui_alignment.cpp` - Refactor to use AlignmentLayout constants
- `src/display/ui_graph.cpp` - Refactor to use GraphLayout constants
- `src/display/ui_network.cpp` - Refactor to use NetworkLayout constants

**Expected Outcome:**
- All `setCursor(10, 50)` become `setCursor(MonitorLayout::TEMP_LABEL_X, MonitorLayout::TEMP_LABEL_Y)`
- All `setTextSize(2)` become `setTextSize(MonitorLayout::HEADER_FONT_SIZE)`
- Compile successfully with NO visual changes to screens

---

### Phase 2: Touchscreen Support (PRIORITY 2)
**Goal:** Enable XPT2046 touchscreen for screen navigation and WiFi setup

**Hardware:**
- XPT2046 touch controller on GPIO 33 (TOUCH_CS)
- Uses same SPI bus as display (HSPI)
- Already defined in pins.h, just needs initialization

**Files to Create:**
- `src/input/touch_handler.h` - Touch event declarations
- `src/input/touch_handler.cpp` - Touch zone detection and actions

**Files to Modify:**
- `src/display/display.h` - Add touch instance to LGFX class
- `src/display/display.cpp` - Initialize XPT2046 in LGFX constructor
- `src/main.cpp` - Add handleTouchInput() to loop()

**Touch Zones:**
```
┌─────────────────────────────────────┐
│ HEADER (tap = WiFi setup mode)     │ Y: 0-25
├─────────────────────────────────────┤
│                                     │
│        MAIN DISPLAY AREA            │
│                                     │
│                                     │
├─────────────────────────────────────┤
│ BOTTOM (tap = cycle screens)        │ Y: 280-320
└─────────────────────────────────────┘
```

**Expected Outcome:**
- Tap top = Enter WiFi AP mode
- Tap bottom = Cycle through display modes
- Button still works (backward compatible)

---

### Phase 3: Data Logging (PRIORITY 3)
**Goal:** Log sensor data to CSV files on SD card

**Files to Create:**
- `src/logging/data_logger.h` - Logger interface
- `src/logging/data_logger.cpp` - CSV writing implementation

**Files to Modify:**
- `src/main.cpp` - Call logger.update() in loop()
- `src/web/web_handlers.cpp` - Add /api/logs/download endpoint

**Log Format:**
```csv
Timestamp,TempX,TempYL,TempYR,TempZ,PSU_Voltage,Fan_RPM,Machine_State
2025-11-20 14:30:00,42.3,38.1,45.2,40.0,12.1,2450,Idle
2025-11-20 14:30:10,43.1,39.0,46.0,41.2,12.0,2600,Run
```

**Features:**
- Configurable log interval (default 10 seconds)
- Automatic log rotation (max 10MB per file)
- Web interface to download logs
- Enabled/disabled via settings page

**Expected Outcome:**
- Logs written to `/logs/fluiddash_YYYYMMDD.csv`
- Minimal performance impact (<1ms per log entry)
- Graceful handling of SD card removal

---

## 📋 Implementation Checklist

### Phase 1: Layout Refactoring
- [ ] Create `src/display/ui_layout.h`
  - [ ] MonitorLayout namespace with all constants
  - [ ] AlignmentLayout namespace
  - [ ] GraphLayout namespace
  - [ ] NetworkLayout namespace
- [ ] Refactor `ui_monitor.cpp`
  - [ ] Replace all hard-coded X/Y coordinates
  - [ ] Replace all setTextSize() magic numbers
  - [ ] Test compile
- [ ] Refactor `ui_alignment.cpp`
  - [ ] Replace coordinate calculations
  - [ ] Replace font sizes
  - [ ] Test compile
- [ ] Refactor `ui_graph.cpp`
  - [ ] Replace graph dimensions
  - [ ] Replace axis positions
  - [ ] Test compile
- [ ] Refactor `ui_network.cpp`
  - [ ] Replace status display positions
  - [ ] Replace text sizes
  - [ ] Test compile
- [ ] Build and test all screens visually
- [ ] Commit Phase 1

### Phase 2: Touchscreen Support
- [ ] Add touch library dependency (if needed)
- [ ] Modify `src/display/display.h`
  - [ ] Add `lgfx::Touch_XPT2046` instance
- [ ] Modify `src/display/display.cpp`
  - [ ] Add touch configuration in LGFX constructor
  - [ ] Test touch initialization
- [ ] Create `src/input/touch_handler.h`
  - [ ] Declare handleTouchInput() function
  - [ ] Declare touch zone constants
- [ ] Create `src/input/touch_handler.cpp`
  - [ ] Implement zone detection
  - [ ] Implement screen cycling
  - [ ] Implement WiFi setup trigger
- [ ] Modify `src/main.cpp`
  - [ ] Add handleTouchInput() to loop()
  - [ ] Test debouncing (prevent double-taps)
- [ ] Build and test on hardware
  - [ ] Test tap-to-cycle screens
  - [ ] Test tap-to-enter WiFi setup
  - [ ] Verify button still works
- [ ] Commit Phase 2

### Phase 3: Data Logging
- [ ] Create `src/logging/data_logger.h`
  - [ ] DataLogger class declaration
  - [ ] Configuration options
- [ ] Create `src/logging/data_logger.cpp`
  - [ ] CSV header writing
  - [ ] Data row formatting
  - [ ] File rotation logic
  - [ ] SD card error handling
- [ ] Modify `src/main.cpp`
  - [ ] Initialize logger in setup()
  - [ ] Call logger.update() in loop()
- [ ] Modify `src/web/web_handlers.cpp`
  - [ ] Add /api/logs/list endpoint
  - [ ] Add /api/logs/download endpoint
  - [ ] Add /api/logs/clear endpoint
- [ ] Modify `data/web/settings.html`
  - [ ] Add logging enable/disable toggle
  - [ ] Add log interval setting
  - [ ] Add log download button
- [ ] Build and test
  - [ ] Verify CSV format
  - [ ] Test log rotation
  - [ ] Test web download
  - [ ] Test SD card removal handling
- [ ] Commit Phase 3

### Final Steps
- [ ] Full integration test on hardware
- [ ] Update CLAUDE.md with new features
- [ ] Create comprehensive commit message
- [ ] Push to remote
- [ ] Create pull request

---

## 🔧 Key Implementation Details

### Layout Constants Structure

```cpp
// src/display/ui_layout.h
#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include <Arduino.h>

// ========== COMMON LAYOUT ==========
namespace CommonLayout {
    constexpr int SCREEN_WIDTH = 480;
    constexpr int SCREEN_HEIGHT = 320;
    constexpr int HEADER_HEIGHT = 25;
}

// ========== MONITOR MODE ==========
namespace MonitorLayout {
    // Header
    constexpr int HEADER_FONT_SIZE = 2;
    constexpr int HEADER_TEXT_X = 10;
    constexpr int HEADER_TEXT_Y = 6;
    constexpr int DATETIME_X = 270;
    constexpr int DATETIME_Y = 6;

    // Divider lines
    constexpr int TOP_DIVIDER_Y = 25;
    constexpr int MIDDLE_DIVIDER_Y = 175;
    constexpr int VERTICAL_DIVIDER_X = 240;

    // Left section - Driver temperatures
    constexpr int TEMP_SECTION_X = 10;
    constexpr int TEMP_LABEL_Y = 30;
    constexpr int TEMP_LABEL_FONT_SIZE = 1;
    constexpr int TEMP_START_Y = 50;
    constexpr int TEMP_ROW_SPACING = 30;
    constexpr int TEMP_VALUE_X = 50;
    constexpr int TEMP_VALUE_Y_OFFSET = -3;
    constexpr int TEMP_VALUE_FONT_SIZE = 2;
    constexpr int PEAK_TEMP_X = 140;
    constexpr int PEAK_TEMP_Y_OFFSET = 2;
    constexpr int PEAK_TEMP_FONT_SIZE = 1;

    // Right section - PSU & Fan
    constexpr int RIGHT_SECTION_X = 250;
    constexpr int PSU_LABEL_Y = 30;
    constexpr int PSU_VALUE_Y = 50;
    constexpr int PSU_FONT_SIZE = 3;
    constexpr int FAN_LABEL_Y = 105;
    constexpr int FAN_RPM_Y = 125;
    constexpr int FAN_SPEED_Y = 155;
    constexpr int FAN_VALUE_FONT_SIZE = 2;

    // Bottom section - Machine status (if FluidNC connected)
    constexpr int STATUS_Y = 185;
    constexpr int STATUS_FONT_SIZE = 2;
    constexpr int STATUS_LABEL_X = 10;
    constexpr int STATUS_VALUE_X = 90;
    constexpr int COORD_ROW_1_Y = 210;
    constexpr int COORD_ROW_2_Y = 240;
    constexpr int COORD_ROW_3_Y = 270;
    constexpr int COORD_FONT_SIZE = 2;
}

// ========== ALIGNMENT MODE ==========
namespace AlignmentLayout {
    constexpr int TITLE_X = 140;
    constexpr int TITLE_Y = 6;
    constexpr int TITLE_FONT_SIZE = 2;

    constexpr int SUBTITLE_X = 150;
    constexpr int SUBTITLE_Y = 40;
    constexpr int SUBTITLE_FONT_SIZE = 2;

    // 3-axis display (large)
    constexpr int COORD_3AXIS_START_X = 40;
    constexpr int COORD_3AXIS_START_Y = 90;
    constexpr int COORD_3AXIS_SPACING = 65;
    constexpr int COORD_3AXIS_FONT_SIZE = 5;

    // 4-axis display (compact)
    constexpr int COORD_4AXIS_START_X = 40;
    constexpr int COORD_4AXIS_START_Y = 75;
    constexpr int COORD_4AXIS_SPACING = 45;
    constexpr int COORD_4AXIS_FONT_SIZE = 4;

    constexpr int MACHINE_POS_Y = 265;
    constexpr int MACHINE_POS_FONT_SIZE = 1;
}

// ========== GRAPH MODE ==========
namespace GraphLayout {
    constexpr int TITLE_Y = 6;
    constexpr int TITLE_FONT_SIZE = 2;

    constexpr int GRAPH_X = 40;
    constexpr int GRAPH_Y = 60;
    constexpr int GRAPH_WIDTH = 400;
    constexpr int GRAPH_HEIGHT = 200;

    constexpr int LEGEND_X = 40;
    constexpr int LEGEND_Y = 270;
    constexpr int LEGEND_FONT_SIZE = 1;
    constexpr int LEGEND_SPACING = 100;
}

// ========== NETWORK MODE ==========
namespace NetworkLayout {
    constexpr int TITLE_X = 130;
    constexpr int TITLE_Y = 6;
    constexpr int TITLE_FONT_SIZE = 2;

    constexpr int STATUS_START_Y = 50;
    constexpr int STATUS_ROW_SPACING = 30;
    constexpr int STATUS_LABEL_X = 10;
    constexpr int STATUS_VALUE_X = 150;
    constexpr int STATUS_FONT_SIZE = 2;
}

#endif // UI_LAYOUT_H
```

### Touch Initialization Code

```cpp
// Add to src/display/display.h (after line 20, in private section)
lgfx::Touch_XPT2046 _touch_instance;

// Add to src/display/display.cpp (in LGFX constructor, after backlight config)
{
    auto cfg = _touch_instance.config();
    cfg.x_min = 0;
    cfg.x_max = 319;
    cfg.y_min = 0;
    cfg.y_max = 479;
    cfg.pin_int  = -1;           // No interrupt pin
    cfg.pin_cs   = TOUCH_CS;     // GPIO 33
    cfg.pin_rst  = -1;           // No reset pin
    cfg.spi_host = HSPI_HOST;    // Same SPI bus as display
    cfg.freq = 1000000;          // 1 MHz
    cfg.x_reverse = false;
    cfg.y_reverse = false;
    cfg.offset_rotation = 0;
    _touch_instance.config(cfg);
    _panel_instance.setTouch(&_touch_instance);
}
```

### Touch Handler Implementation

```cpp
// src/input/touch_handler.h
#ifndef TOUCH_HANDLER_H
#define TOUCH_HANDLER_H

#include <Arduino.h>

// Touch zone definitions
#define TOUCH_ZONE_HEADER_Y_MAX 25
#define TOUCH_ZONE_FOOTER_Y_MIN 280
#define TOUCH_DEBOUNCE_MS 300

void handleTouchInput();
void cycleModeForward();

#endif
```

```cpp
// src/input/touch_handler.cpp
#include "touch_handler.h"
#include "display/display.h"
#include "display/ui_modes.h"

static unsigned long lastTouchTime = 0;

void handleTouchInput() {
    uint16_t x, y;

    // Check if screen is touched
    if (gfx.getTouch(&x, &y)) {
        // Debounce - ignore touches within 300ms of last touch
        unsigned long now = millis();
        if (now - lastTouchTime < TOUCH_DEBOUNCE_MS) {
            return;
        }
        lastTouchTime = now;

        // Detect touch zones
        if (y < TOUCH_ZONE_HEADER_Y_MAX) {
            // Header tap - enter WiFi setup mode
            enterSetupMode();
        } else if (y > TOUCH_ZONE_FOOTER_Y_MIN) {
            // Footer tap - cycle to next screen
            cycleModeForward();
            drawScreen();
        }
    }
}

void cycleModeForward() {
    extern DisplayMode currentMode;
    currentMode = (DisplayMode)((currentMode + 1) % 4);  // 4 display modes
}
```

### Data Logger Implementation

```cpp
// src/logging/data_logger.h
#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <Arduino.h>
#include <SD.h>

class DataLogger {
public:
    DataLogger();

    void begin();
    void update();  // Call from loop()
    void setEnabled(bool enabled);
    void setInterval(unsigned long intervalMs);

    bool isEnabled() { return _enabled; }
    unsigned long getInterval() { return _logInterval; }

private:
    void writeLogEntry();
    void rotateLogFile();
    String getCurrentLogFilename();

    bool _enabled;
    unsigned long _logInterval;
    unsigned long _lastLogTime;
    File _currentLogFile;

    static constexpr unsigned long DEFAULT_INTERVAL = 10000;  // 10 seconds
    static constexpr size_t MAX_LOG_SIZE = 10 * 1024 * 1024;   // 10 MB
};

extern DataLogger logger;

#endif
```

---

## 🚨 Session Recovery Instructions

**If session hangs up, read this file and PROGRESS_LOG.md to resume.**

### Quick Recovery Steps:
1. Read PROGRESS_LOG.md to see what was completed
2. Read this file to understand current phase
3. Check git status to see uncommitted changes
4. Continue from next uncompleted task in checklist above
5. Update PROGRESS_LOG.md after each major step

### Critical Files:
- `IMPLEMENTATION_GUIDE.md` (this file) - What to do
- `PROGRESS_LOG.md` - What's been done
- Todo list in session - Current status

---

## 📝 Notes for Future Sessions

### Compilation Commands:
```bash
# Build
pio run -e esp32dev

# Upload
pio run -e esp32dev -t upload

# Monitor
pio device monitor -b 115200
```

### Git Workflow:
```bash
# Check status
git status

# Stage changes
git add <files>

# Commit
git commit -m "feat: Description of changes"

# Push (requires Claude Code in VS Code)
git push -u origin feature/touchscreen-and-layout
```

### Testing Checklist:
- [ ] Code compiles without errors
- [ ] All display modes render correctly
- [ ] Touch input responds
- [ ] Button still works
- [ ] Data logs to SD card
- [ ] Web interface accessible
- [ ] No memory leaks (check free heap)
- [ ] No watchdog timeouts

---

**End of Implementation Guide**
