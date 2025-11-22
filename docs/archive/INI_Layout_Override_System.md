# INI-Based Layout Override System - Implementation Guide

**Created:** 2025-01-22
**Status:** Design Complete - Ready for Implementation
**Target Version:** v0.4.0
**Estimated Effort:** 6-8 hours total (phased approach)

---

## Table of Contents

1. [Concept Overview](#concept-overview)
2. [Architecture Design](#architecture-design)
3. [Salvaging the Visual Editor](#salvaging-the-visual-editor)
4. [Implementation Phases](#implementation-phases)
5. [Technical Specifications](#technical-specifications)
6. [File Structure](#file-structure)
7. [Code Examples](#code-examples)
8. [Testing Plan](#testing-plan)
9. [Future Enhancements](#future-enhancements)

---

## Concept Overview

### The Problem

Users want to customize screen layouts (font sizes, positions, spacing) without:
- Recompiling firmware
- Editing C++ code
- Risking boot failures
- Complex JSON configurations

### The Solution

**INI-style override files** that provide a sweet spot between hard-coded reliability and dynamic flexibility:

```ini
# /layouts/monitor.ini
[Header]
x=10
y=15
fontSize=3

[Temperature]
labelX=10
labelY=55
valueX=120
```

### Key Principles

1. **Optional** - If file missing/corrupt, use hard-coded defaults
2. **Boot-safe** - Loads AFTER boot completes, never blocks
3. **Simple** - Line-by-line text parsing, no JSON overhead
4. **Live reload** - Web API to reload without rebooting device
5. **Visual editing** - Repurpose existing editor.html for INI workflow

---

## Architecture Design

### System Components

```
┌─────────────────────────────────────────────────────┐
│                    User Interface                    │
│  ┌──────────────────┐      ┌────────────────────┐  │
│  │ Simple Settings  │      │  Visual Editor     │  │
│  │ Form (/settings) │      │  (/editor)         │  │
│  │                  │      │                    │  │
│  │ Font: [__2__]    │      │  ┌──────────────┐ │  │
│  │ PosX: [__10__]   │      │  │ 480×320      │ │  │
│  │ [Save]           │      │  │ Canvas       │ │  │
│  └──────────────────┘      │  │ Preview      │ │  │
│                            │  └──────────────┘ │  │
│                            │  Properties Panel│  │
│                            └────────────────────┘  │
└─────────────────────────────────────────────────────┘
                         │
                         ▼ HTTP POST
┌─────────────────────────────────────────────────────┐
│              Backend API (ESP32)                     │
│  /api/layout/defaults      - Get hard-coded values  │
│  /api/layout/overrides/:screen - Get current INI    │
│  /api/layout/save          - Save INI to SD          │
│  /api/layout/reload        - Reload and redraw       │
└─────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────┐
│           Runtime Layout Configuration               │
│  ┌─────────────────────────────────────────────┐   │
│  │ MonitorLayoutConfig monitorLayout;          │   │
│  │   .headerX = MonitorLayout::HEADER_X (10)   │   │
│  │   .headerY = 15  ← Overridden from INI      │   │
│  │   .headerFontSize = 3 ← Overridden          │   │
│  └─────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────┐
│              Hard-Coded Defaults                     │
│  src/display/ui_layout.h                            │
│  namespace MonitorLayout {                          │
│      constexpr int HEADER_X = 10;                   │
│      constexpr int HEADER_Y = 6;  ← Default         │
│      constexpr int HEADER_FONT_SIZE = 2; ← Default  │
│  }                                                   │
└─────────────────────────────────────────────────────┘
```

### Data Flow

**Boot Sequence:**
1. Device initializes with hard-coded defaults
2. After boot completes, `loadLayoutOverrides()` called
3. If `/layouts/monitor.ini` exists, parse and apply overrides
4. If file missing/corrupt, continue with defaults (no error)
5. Screen renders using runtime configuration

**User Edit Workflow:**
1. User opens `/editor` in browser
2. Editor loads defaults via `/api/layout/defaults`
3. Editor loads current overrides via `/api/layout/overrides/monitor`
4. User adjusts values, sees instant canvas preview
5. User clicks Save → POST to `/api/layout/save`
6. Backend writes INI file to SD card
7. User clicks Reload → POST to `/api/layout/reload`
8. Device reloads INI and redraws screen

---

## Salvaging the Visual Editor

### What We Keep (70% of editor.html)

| Component | Current Use (JSON) | New Use (INI) |
|-----------|-------------------|---------------|
| **Canvas (480×320)** | Draw new elements | Preview screen with overrides |
| **Properties Panel** | Edit element properties | Edit override values |
| **File Operations** | Save/load JSON layouts | Save/load INI files |
| **Screen Selector** | Select layout file | Select screen mode (monitor/alignment/etc) |
| **Status Messages** | Operation feedback | Same |
| **UI Styling** | Professional look | Same |

### What We Change

**1. Left Toolbar**

Replace "Add Elements" with "Override Sections":

```html
<!-- OLD: Element type selector -->
<select id="elementType">
    <option value="rect">Rectangle</option>
    <option value="line">Line</option>
</select>
<button onclick="startDrawing()">Draw Element</button>

<!-- NEW: Section selector -->
<select id="sectionSelector" onchange="selectSection()">
    <option value="">-- Select Section --</option>
    <option value="Header">Header</option>
    <option value="Temperature">Temperature Display</option>
    <option value="Position">Position Coordinates</option>
    <option value="PSU">PSU Voltage</option>
</select>
<div id="sectionList">
    <!-- List of sections with override indicators -->
</div>
```

**2. Canvas Interaction**

```javascript
// OLD: Draw mode with mouse drag
function onCanvasMouseDown(e) {
    if (!isDrawing) return;
    drawStart = { x, y };
}

// NEW: Click to select section (optional feature)
function onCanvasClick(e) {
    const { x, y } = getCanvasCoordinates(e);
    const section = detectSectionAtPoint(x, y);
    if (section) {
        selectSection(section);
    }
}
```

**3. Properties Panel**

```html
<!-- OLD: Element properties -->
<div class="property-group">
    <label>Element Type</label>
    <input type="text" value="rect" readonly>
</div>

<!-- NEW: Section overrides with defaults shown -->
<div class="property-group">
    <label>Header X Position</label>
    <input type="number" value="10"
           data-default="10"
           onchange="updateOverride('Header', 'x', this.value)">
    <span class="default-indicator">Default: 10</span>
</div>

<div class="property-group">
    <label>Header Y Position</label>
    <input type="number" value="15"
           data-default="6"
           onchange="updateOverride('Header', 'y', this.value)">
    <span class="default-indicator override-active">⚠️ Default: 6</span>
</div>
```

**4. Canvas Rendering**

Instead of drawing user-created elements, render the actual Monitor screen:

```javascript
function renderMonitorPreview(layout) {
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, 480, 320);

    // Draw header
    ctx.fillStyle = '#000';
    ctx.font = `${layout.Header.fontSize * 8}px Arial`;
    ctx.fillText('FluidDash', layout.Header.x, layout.Header.y + 15);

    // Draw temperature labels
    ctx.font = `${layout.Temperature.labelFontSize * 8}px Arial`;
    ctx.fillText('X-Axis:', layout.Temperature.labelX, layout.Temperature.labelY);

    // ... render entire screen layout
}
```

---

## Implementation Phases

### Phase 0: Proof of Concept (1-2 hours)

**Goal:** Verify the approach works with minimal implementation

**Tasks:**
1. Create `src/display/layout_config.h` with MonitorLayoutConfig struct
2. Implement basic INI parser for Header section only
3. Modify `drawMonitorMode()` to use runtime values
4. Test with manual INI file

**Success Criteria:**
- Device boots normally if INI missing
- Device loads overrides if INI present
- Header position/font changes work
- No memory leaks or crashes

**STOP POINT:** Validate the concept before proceeding

---

### Phase 1: Backend Foundation (2-3 hours)

**Goal:** Complete backend INI system for Monitor mode

**Tasks:**

1. **Expand layout_config.h**
   - Add all Monitor sections (Header, Temperature, Position, PSU, Fan)
   - ~30-40 overridable values total

2. **Implement full INI parser**
   - Section detection `[Header]`
   - Key-value parsing `x=10`
   - Error handling for malformed lines
   - Default fallback on parse errors

3. **Add Web API endpoints**
   - `GET /api/layout/defaults` - Return hard-coded constants as JSON
   - `GET /api/layout/overrides/:screen` - Return current INI content
   - `POST /api/layout/save` - Write INI to SD card
   - `POST /api/layout/reload` - Reload INI and redraw screen

4. **Update all Monitor drawing code**
   - Replace `MonitorLayout::HEADER_X` with `monitorLayout.headerX`
   - Ensure all sections use runtime values

**Files Modified:**
- `src/display/layout_config.h` (new)
- `src/display/layout_config.cpp` (new)
- `src/display/ui_monitor.cpp` (modified)
- `src/web/web_handlers.h` (modified)
- `src/web/web_handlers.cpp` (modified)
- `src/main.cpp` (add loadLayoutOverrides() call)

**Success Criteria:**
- All Monitor sections overridable
- API endpoints work (test with curl)
- Reload works without device restart
- Invalid INI files don't crash device

**STOP POINT:** Test thoroughly before building UI

---

### Phase 2: Simple Settings Form (1 hour)

**Goal:** Quick, functional UI for basic adjustments

**Tasks:**

1. Add layout section to `/settings` page:

```html
<h3>Monitor Screen Layout</h3>
<div class="form-group">
    <label>Header Font Size</label>
    <input type="number" id="header_fontSize" min="1" max="5" value="2">
    <span class="help-text">Default: 2</span>
</div>

<div class="form-group">
    <label>Temperature Label X</label>
    <input type="number" id="temp_labelX" min="0" max="480" value="10">
    <span class="help-text">Default: 10</span>
</div>

<!-- 10-15 most common overrides -->

<button onclick="saveLayoutOverrides()">Save Layout</button>
<button onclick="resetLayoutDefaults()">Reset to Defaults</button>
```

2. Add JavaScript handlers:

```javascript
async function saveLayoutOverrides() {
    const overrides = {
        Header: {
            fontSize: parseInt(document.getElementById('header_fontSize').value)
        },
        Temperature: {
            labelX: parseInt(document.getElementById('temp_labelX').value)
        }
    };

    const ini = convertToINI(overrides);
    await fetch('/api/layout/save', {
        method: 'POST',
        body: JSON.stringify({ screen: 'monitor', content: ini })
    });

    await fetch('/api/layout/reload', { method: 'POST' });
    showStatus('Layout updated!');
}
```

**Success Criteria:**
- Form loads current values
- Save button writes INI
- Reload button triggers screen update
- Reset button clears overrides

**STOP POINT:** This is a functional release - visual editor is optional

---

### Phase 3: Visual Editor (3-4 hours)

**Goal:** Polished visual editing experience

**Tasks:**

1. **Adapt editor.html structure**
   - Change screen selector to mode selector (monitor/alignment/graph/etc)
   - Replace element list with section list
   - Modify properties panel for override editing

2. **Implement JavaScript canvas rendering**
   - Create `renderMonitorScreen(layout)` function
   - Recreate Monitor mode drawing logic in JS
   - Match LCD output as closely as possible

3. **Add override comparison UI**
   - Show default values alongside current
   - Highlight overridden values with ⚠️ indicator
   - Color-code sections (default=green, overridden=orange)

4. **Implement live preview**
   - On value change, immediately re-render canvas
   - Show validation errors (off-screen, invalid range)
   - Debounce rendering for performance

5. **Add section highlighting**
   - Click canvas area → select corresponding section
   - Hover section → highlight area on canvas
   - Visual feedback for what you're editing

**Files Modified:**
- `data/web/editor.html` (major refactoring)
- `src/web/web_handlers.cpp` (serve editor at `/editor`)

**Success Criteria:**
- Canvas accurately previews Monitor screen
- Value changes update canvas instantly
- Section selection works
- Save/load/reset work correctly
- Works offline (JavaScript-only preview)

---

### Phase 4: Expand to Other Screens (1 hour per screen)

**Goal:** Apply system to all screen modes

**Tasks:**

1. **Alignment Mode**
   - Create AlignmentLayoutConfig struct
   - Add /layouts/alignment.ini support
   - Add to editor dropdown

2. **Graph Mode**
   - Create GraphLayoutConfig struct
   - Add /layouts/graph.ini support
   - Add to editor

3. **Network Mode**
   - Create NetworkLayoutConfig struct
   - Add /layouts/network.ini support
   - Add to editor

4. **Storage Mode**
   - Create StorageLayoutConfig struct
   - Add /layouts/storage.ini support
   - Add to editor

**Success Criteria:**
- All 5 screen modes support overrides
- Editor can switch between modes
- Each mode has its own INI file

---

## Technical Specifications

### Backend: Runtime Layout Configuration

**File:** `src/display/layout_config.h`

```cpp
#ifndef LAYOUT_CONFIG_H
#define LAYOUT_CONFIG_H

#include <Arduino.h>
#include "ui_layout.h"

// ========== Monitor Mode Runtime Configuration ==========

struct MonitorLayoutConfig {
    // Header section
    int headerX = MonitorLayout::HEADER_TEXT_X;
    int headerY = MonitorLayout::HEADER_TEXT_Y;
    int headerFontSize = MonitorLayout::HEADER_FONT_SIZE;
    int datetimeX = MonitorLayout::DATETIME_X;
    int datetimeY = MonitorLayout::DATETIME_Y;

    // Temperature section
    int tempSectionX = MonitorLayout::TEMP_SECTION_X;
    int tempLabelY = MonitorLayout::TEMP_LABEL_Y;
    int tempLabelFontSize = MonitorLayout::TEMP_LABEL_FONT_SIZE;
    int tempStartY = MonitorLayout::TEMP_START_Y;
    int tempRowSpacing = MonitorLayout::TEMP_ROW_SPACING;
    int tempValueX = MonitorLayout::TEMP_VALUE_X;
    int tempValueFontSize = MonitorLayout::TEMP_VALUE_FONT_SIZE;

    // Position section
    int statusY = MonitorLayout::STATUS_Y;
    int statusFontSize = MonitorLayout::STATUS_FONT_SIZE;
    int coordRow1Y = MonitorLayout::COORD_ROW_1_Y;
    int coordRow2Y = MonitorLayout::COORD_ROW_2_Y;
    int coordFontSize = MonitorLayout::COORD_FONT_SIZE;

    // PSU section
    int psuLabelY = MonitorLayout::PSU_LABEL_Y;
    int psuValueY = MonitorLayout::PSU_VALUE_Y;
    int psuFontSize = MonitorLayout::PSU_FONT_SIZE;

    // Fan section
    int fanLabelY = MonitorLayout::FAN_LABEL_Y;
    int fanRpmY = MonitorLayout::FAN_RPM_Y;
    int fanSpeedY = MonitorLayout::FAN_SPEED_Y;
    int fanValueFontSize = MonitorLayout::FAN_VALUE_FONT_SIZE;
};

// Global instance
extern MonitorLayoutConfig monitorLayout;

// ========== Functions ==========

// Load INI overrides from SD card (call after boot)
void loadLayoutOverrides();

// Reload overrides and redraw screen (for web API)
void reloadLayoutOverrides();

// Get defaults as JSON (for web API)
String getLayoutDefaultsJSON(const char* screenMode);

// Get current overrides as INI text (for web API)
String getLayoutOverridesINI(const char* screenMode);

// Save overrides to SD card (for web API)
bool saveLayoutOverrides(const char* screenMode, const char* iniContent);

#endif // LAYOUT_CONFIG_H
```

---

**File:** `src/display/layout_config.cpp`

```cpp
#include "layout_config.h"
#include "storage_manager.h"
#include "config/config.h"
#include <ArduinoJson.h>

// Global instance with defaults from ui_layout.h
MonitorLayoutConfig monitorLayout;

// ========== INI Parser ==========

void loadLayoutOverrides() {
    const char* filename = "/layouts/monitor.ini";

    if (!storage.exists(filename)) {
        Serial.println("[Layout] No override file, using defaults");
        return;
    }

    File file = storage.openFile(filename, FILE_READ);
    if (!file) {
        Serial.println("[Layout] Failed to open override file");
        return;
    }

    Serial.printf("[Layout] Loading overrides from %s\n", filename);

    String currentSection = "";
    int lineCount = 0;
    int overrideCount = 0;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        lineCount++;

        // Skip empty lines and comments
        if (line.length() == 0 || line.startsWith("#")) {
            continue;
        }

        // Section header: [Header]
        if (line.startsWith("[") && line.endsWith("]")) {
            currentSection = line.substring(1, line.length() - 1);
            currentSection.trim();
            continue;
        }

        // Key-value pair: x=10
        int equalPos = line.indexOf('=');
        if (equalPos < 0) {
            Serial.printf("[Layout] Line %d: Invalid format (no '=')\n", lineCount);
            continue;
        }

        String key = line.substring(0, equalPos);
        String valueStr = line.substring(equalPos + 1);
        key.trim();
        valueStr.trim();

        int value = valueStr.toInt();

        // Apply override based on section and key
        if (currentSection == "Header") {
            if (key == "x") monitorLayout.headerX = value;
            else if (key == "y") monitorLayout.headerY = value;
            else if (key == "fontSize") monitorLayout.headerFontSize = value;
            else if (key == "datetimeX") monitorLayout.datetimeX = value;
            else if (key == "datetimeY") monitorLayout.datetimeY = value;
            else {
                Serial.printf("[Layout] Unknown key: [%s] %s\n", currentSection.c_str(), key.c_str());
                continue;
            }
            overrideCount++;
        }
        else if (currentSection == "Temperature") {
            if (key == "sectionX") monitorLayout.tempSectionX = value;
            else if (key == "labelY") monitorLayout.tempLabelY = value;
            else if (key == "labelFontSize") monitorLayout.tempLabelFontSize = value;
            else if (key == "startY") monitorLayout.tempStartY = value;
            else if (key == "rowSpacing") monitorLayout.tempRowSpacing = value;
            else if (key == "valueX") monitorLayout.tempValueX = value;
            else if (key == "valueFontSize") monitorLayout.tempValueFontSize = value;
            else {
                Serial.printf("[Layout] Unknown key: [%s] %s\n", currentSection.c_str(), key.c_str());
                continue;
            }
            overrideCount++;
        }
        // ... add other sections (Position, PSU, Fan)
    }

    file.close();
    Serial.printf("[Layout] Loaded %d overrides from %d lines\n", overrideCount, lineCount);
}

// ========== Reload Function ==========

void reloadLayoutOverrides() {
    // Reset to defaults
    monitorLayout = MonitorLayoutConfig();

    // Reload overrides
    loadLayoutOverrides();

    // Trigger screen redraw
    extern DisplayMode currentMode;
    extern void drawScreen();
    if (currentMode == MODE_MONITOR) {
        drawScreen();
    }
}

// ========== Web API Functions ==========

String getLayoutDefaultsJSON(const char* screenMode) {
    JsonDocument doc;

    if (strcmp(screenMode, "monitor") == 0) {
        JsonObject header = doc["Header"].to<JsonObject>();
        header["x"] = MonitorLayout::HEADER_TEXT_X;
        header["y"] = MonitorLayout::HEADER_TEXT_Y;
        header["fontSize"] = MonitorLayout::HEADER_FONT_SIZE;
        header["datetimeX"] = MonitorLayout::DATETIME_X;
        header["datetimeY"] = MonitorLayout::DATETIME_Y;

        JsonObject temp = doc["Temperature"].to<JsonObject>();
        temp["sectionX"] = MonitorLayout::TEMP_SECTION_X;
        temp["labelY"] = MonitorLayout::TEMP_LABEL_Y;
        temp["labelFontSize"] = MonitorLayout::TEMP_LABEL_FONT_SIZE;
        temp["startY"] = MonitorLayout::TEMP_START_Y;
        temp["rowSpacing"] = MonitorLayout::TEMP_ROW_SPACING;
        temp["valueX"] = MonitorLayout::TEMP_VALUE_X;
        temp["valueFontSize"] = MonitorLayout::TEMP_VALUE_FONT_SIZE;

        // ... add other sections
    }

    String output;
    serializeJson(doc, output);
    return output;
}

String getLayoutOverridesINI(const char* screenMode) {
    const char* filename = "/layouts/monitor.ini"; // TODO: support other modes

    if (!storage.exists(filename)) {
        return ""; // No overrides
    }

    File file = storage.openFile(filename, FILE_READ);
    if (!file) return "";

    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }
    file.close();

    return content;
}

bool saveLayoutOverrides(const char* screenMode, const char* iniContent) {
    const char* filename = "/layouts/monitor.ini"; // TODO: support other modes

    // Ensure directory exists
    if (!storage.exists("/layouts")) {
        SD.mkdir("/layouts");
    }

    File file = storage.openFile(filename, FILE_WRITE);
    if (!file) {
        Serial.println("[Layout] Failed to open file for writing");
        return false;
    }

    file.print(iniContent);
    file.close();

    Serial.printf("[Layout] Saved overrides to %s\n", filename);
    return true;
}
```

---

### Backend: Web API Endpoints

**File:** `src/web/web_handlers.cpp`

Add these handlers:

```cpp
// GET /api/layout/defaults?screen=monitor
void handleAPILayoutDefaults() {
    String screenMode = server.arg("screen");
    if (screenMode.length() == 0) {
        server.send(400, "application/json", "{\"error\":\"Missing screen parameter\"}");
        return;
    }

    String json = getLayoutDefaultsJSON(screenMode.c_str());
    server.send(200, "application/json", json);
}

// GET /api/layout/overrides?screen=monitor
void handleAPILayoutOverrides() {
    String screenMode = server.arg("screen");
    if (screenMode.length() == 0) {
        server.send(400, "text/plain", "Missing screen parameter");
        return;
    }

    String ini = getLayoutOverridesINI(screenMode.c_str());
    if (ini.length() == 0) {
        server.send(200, "text/plain", "# No overrides\n");
    } else {
        server.send(200, "text/plain", ini);
    }
}

// POST /api/layout/save
// Body: { "screen": "monitor", "content": "[Header]\nx=10\n..." }
void handleAPILayoutSave() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    const char* screenMode = doc["screen"];
    const char* content = doc["content"];

    if (!screenMode || !content) {
        server.send(400, "application/json", "{\"error\":\"Missing screen or content\"}");
        return;
    }

    bool success = saveLayoutOverrides(screenMode, content);
    if (success) {
        server.send(200, "application/json", "{\"success\":true}");
    } else {
        server.send(500, "application/json", "{\"error\":\"Failed to save\"}");
    }
}

// POST /api/layout/reload
void handleAPILayoutReload() {
    reloadLayoutOverrides();
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Layout reloaded\"}");
}
```

Register in `setupWebServer()`:

```cpp
server.on("/api/layout/defaults", HTTP_GET, handleAPILayoutDefaults);
server.on("/api/layout/overrides", HTTP_GET, handleAPILayoutOverrides);
server.on("/api/layout/save", HTTP_POST, handleAPILayoutSave);
server.on("/api/layout/reload", HTTP_POST, handleAPILayoutReload);
```

---

### Frontend: Visual Editor Changes

**Key Modifications to editor.html:**

```javascript
// ============================================
// STATE MANAGEMENT
// ============================================
let currentScreenMode = 'monitor'; // monitor, alignment, graph, network, storage
let defaultLayout = {};  // Hard-coded defaults from device
let currentLayout = {};  // Current values (defaults + overrides)

// ============================================
// LOAD LAYOUT DATA
// ============================================
async function loadLayoutData(screenMode) {
    try {
        // Load defaults
        const defaultsRes = await fetch(`/api/layout/defaults?screen=${screenMode}`);
        defaultLayout = await defaultsRes.json();

        // Load current overrides
        const overridesRes = await fetch(`/api/layout/overrides?screen=${screenMode}`);
        const iniText = await overridesRes.text();

        // Parse INI and merge with defaults
        const overrides = parseINI(iniText);
        currentLayout = deepMerge(defaultLayout, overrides);

        updateSectionList();
        renderCanvasPreview();
        showStatus('Layout loaded', 'success');
    } catch (error) {
        showStatus('Error loading layout: ' + error.message, 'error');
    }
}

// ============================================
// INI PARSING
// ============================================
function parseINI(iniText) {
    const result = {};
    let currentSection = null;

    iniText.split('\n').forEach(line => {
        line = line.trim();
        if (!line || line.startsWith('#')) return;

        // Section: [Header]
        if (line.startsWith('[') && line.endsWith(']')) {
            currentSection = line.slice(1, -1);
            result[currentSection] = {};
            return;
        }

        // Key-value: x=10
        const match = line.match(/^(\w+)\s*=\s*(.+)$/);
        if (match && currentSection) {
            const [, key, value] = match;
            result[currentSection][key] = parseInt(value) || value;
        }
    });

    return result;
}

// ============================================
// CANVAS RENDERING (Monitor Mode Example)
// ============================================
function renderMonitorPreview() {
    const layout = currentLayout;

    // Clear canvas
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, 480, 320);

    // Draw header
    ctx.fillStyle = '#FFF';
    ctx.font = `${layout.Header.fontSize * 8}px Arial`;
    ctx.fillText('FluidDash', layout.Header.x, layout.Header.y + 15);

    ctx.font = `${layout.Header.fontSize * 8}px Arial`;
    ctx.fillText('2025-01-22 14:30', layout.Header.datetimeX, layout.Header.datetimeY + 15);

    // Draw divider lines
    ctx.strokeStyle = '#888';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(0, 25);
    ctx.lineTo(480, 25);
    ctx.stroke();

    // Draw temperature labels
    ctx.font = `${layout.Temperature.labelFontSize * 8}px Arial`;
    const tempLabels = ['X-Axis:', 'Y-Left:', 'Y-Right:', 'Z-Axis:'];
    tempLabels.forEach((label, i) => {
        const y = layout.Temperature.startY + (i * layout.Temperature.rowSpacing);
        ctx.fillText(label, layout.Temperature.sectionX, y);

        // Temperature value
        ctx.font = `${layout.Temperature.valueFontSize * 8}px Arial`;
        ctx.fillStyle = '#0F0';
        ctx.fillText('26.5°C', layout.Temperature.valueX, y - 3);
        ctx.fillStyle = '#FFF';
    });

    // ... continue rendering other sections
}

// ============================================
// SECTION LIST UI
// ============================================
function updateSectionList() {
    const list = document.getElementById('sectionList');
    const sections = Object.keys(currentLayout);

    list.innerHTML = sections.map(section => {
        const hasOverrides = hasAnyOverride(section);
        const icon = hasOverrides ? '⚠️' : '✓';
        const cssClass = hasOverrides ? 'override-active' : '';

        return `
            <div class="section-item ${cssClass}" onclick="selectSection('${section}')">
                <span>${icon} ${section}</span>
            </div>
        `;
    }).join('');
}

function hasAnyOverride(section) {
    const defaults = defaultLayout[section];
    const current = currentLayout[section];

    for (let key in current) {
        if (current[key] !== defaults[key]) {
            return true;
        }
    }
    return false;
}

// ============================================
// PROPERTIES PANEL
// ============================================
function selectSection(sectionName) {
    const content = document.getElementById('propertiesContent');
    const section = currentLayout[sectionName];
    const defaults = defaultLayout[sectionName];

    const html = Object.keys(section).map(key => {
        const current = section[key];
        const defaultVal = defaults[key];
        const isOverridden = current !== defaultVal;
        const indicator = isOverridden ? `<span class="override-warning">⚠️ Default: ${defaultVal}</span>` : `<span class="default-ok">Default: ${defaultVal}</span>`;

        return `
            <div class="property-group">
                <label class="property-label">${key}</label>
                <input type="number"
                       value="${current}"
                       onchange="updateLayoutValue('${sectionName}', '${key}', parseInt(this.value))"
                       ${getValidationAttrs(sectionName, key)}>
                ${indicator}
            </div>
        `;
    }).join('');

    content.innerHTML = `
        <h4>${sectionName}</h4>
        ${html}
        <button class="btn-secondary" onclick="resetSection('${sectionName}')">
            Reset Section to Defaults
        </button>
    `;
}

function updateLayoutValue(section, key, value) {
    currentLayout[section][key] = value;
    renderMonitorPreview();
    updateSectionList();
}

function resetSection(sectionName) {
    currentLayout[sectionName] = { ...defaultLayout[sectionName] };
    selectSection(sectionName);
    renderMonitorPreview();
    updateSectionList();
}

// ============================================
// SAVE FUNCTIONALITY
// ============================================
async function saveLayout() {
    // Convert currentLayout to INI format
    const ini = convertToINI(currentLayout);

    try {
        showStatus('Saving...', 'info');
        const response = await fetch('/api/layout/save', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                screen: currentScreenMode,
                content: ini
            })
        });

        if (response.ok) {
            showStatus('✅ Saved successfully!', 'success');
        } else {
            showStatus('❌ Save failed', 'error');
        }
    } catch (error) {
        showStatus('Error: ' + error.message, 'error');
    }
}

async function reloadLayout() {
    try {
        showStatus('Reloading screen...', 'info');
        await fetch('/api/layout/reload', { method: 'POST' });
        showStatus('✅ Screen reloaded!', 'success');
    } catch (error) {
        showStatus('Error: ' + error.message, 'error');
    }
}

function convertToINI(layout) {
    let ini = '# FluidDash Layout Overrides\n';
    ini += `# Screen: ${currentScreenMode}\n`;
    ini += `# Generated: ${new Date().toISOString()}\n\n`;

    for (let section in layout) {
        const defaults = defaultLayout[section];
        const current = layout[section];

        // Only write overridden values
        const overrides = {};
        for (let key in current) {
            if (current[key] !== defaults[key]) {
                overrides[key] = current[key];
            }
        }

        // Skip section if no overrides
        if (Object.keys(overrides).length === 0) continue;

        ini += `[${section}]\n`;
        for (let key in overrides) {
            ini += `${key}=${overrides[key]}\n`;
        }
        ini += '\n';
    }

    return ini;
}
```

---

## File Structure

```
FluidDash-v02/
├── data/
│   ├── layouts/                    # INI override files (optional, on SD card)
│   │   ├── monitor.ini
│   │   ├── alignment.ini
│   │   ├── graph.ini
│   │   ├── network.ini
│   │   └── storage.ini
│   │
│   └── web/
│       ├── settings.html           # Simple form for quick overrides
│       └── editor.html             # Visual editor (adapted from JSON editor)
│
├── src/
│   ├── display/
│   │   ├── ui_layout.h             # Hard-coded defaults (unchanged)
│   │   ├── layout_config.h         # NEW: Runtime configuration structs
│   │   ├── layout_config.cpp       # NEW: INI parser and loader
│   │   ├── ui_monitor.cpp          # MODIFIED: Use monitorLayout.headerX
│   │   ├── ui_alignment.cpp        # MODIFIED: Use alignmentLayout.*
│   │   └── ...
│   │
│   ├── web/
│   │   ├── web_handlers.h          # MODIFIED: Add layout API declarations
│   │   └── web_handlers.cpp        # MODIFIED: Add layout API handlers
│   │
│   └── main.cpp                    # MODIFIED: Call loadLayoutOverrides()
```

---

## Code Examples

### Example INI File

**File:** `/layouts/monitor.ini`

```ini
# FluidDash Monitor Screen Layout Overrides
# All values are optional - delete line to use default

[Header]
# Move header down slightly and make font bigger
y=15
fontSize=3

[Temperature]
# Shift temperature display to the right
labelX=20
valueX=130
# Increase spacing between rows
rowSpacing=35

[Position]
# Make position coordinates bigger
fontSize=3

# PSU and Fan sections not overridden - using defaults
```

### Usage in Drawing Code

**File:** `src/display/ui_monitor.cpp`

```cpp
// BEFORE (hard-coded)
void drawMonitorMode() {
    gfx.setTextSize(MonitorLayout::HEADER_FONT_SIZE);
    gfx.setCursor(MonitorLayout::HEADER_TEXT_X, MonitorLayout::HEADER_TEXT_Y);
    gfx.print("FluidDash");
}

// AFTER (runtime configurable)
void drawMonitorMode() {
    gfx.setTextSize(monitorLayout.headerFontSize);
    gfx.setCursor(monitorLayout.headerX, monitorLayout.headerY);
    gfx.print("FluidDash");
}
```

---

## Testing Plan

### Phase 0 Testing (Proof of Concept)

**Manual Tests:**
1. ✅ Boot without INI file → uses defaults
2. ✅ Boot with valid INI → loads overrides
3. ✅ Boot with malformed INI → ignores errors, uses defaults
4. ✅ Header position/font changes visible
5. ✅ No memory leaks (check `ESP.getFreeHeap()`)
6. ✅ No watchdog timeouts

### Phase 1 Testing (Backend)

**API Tests (curl):**
```bash
# Get defaults
curl http://192.168.1.100/api/layout/defaults?screen=monitor

# Get current overrides
curl http://192.168.1.100/api/layout/overrides?screen=monitor

# Save overrides
curl -X POST http://192.168.1.100/api/layout/save \
  -H "Content-Type: application/json" \
  -d '{"screen":"monitor","content":"[Header]\ny=15\nfontSize=3\n"}'

# Reload screen
curl -X POST http://192.168.1.100/api/layout/reload
```

**Functional Tests:**
1. ✅ All Monitor sections overridable
2. ✅ Invalid values don't crash
3. ✅ Reload works without reboot
4. ✅ File corruption doesn't break boot
5. ✅ SD card removal handled gracefully

### Phase 2 Testing (Simple Form)

**User Flow Tests:**
1. ✅ Form loads current values
2. ✅ Save button writes INI
3. ✅ Reload button updates screen immediately
4. ✅ Reset button clears overrides
5. ✅ Default indicators show correctly

### Phase 3 Testing (Visual Editor)

**Visual Tests:**
1. ✅ Canvas preview matches LCD output
2. ✅ Value changes update canvas instantly
3. ✅ Section selection highlights correctly
4. ✅ Override indicators accurate
5. ✅ Save/load/reset work
6. ✅ Validation prevents off-screen values

### Regression Testing

After each phase, verify:
- ✅ All 5 display modes still work
- ✅ Touchscreen navigation works
- ✅ Data logging works
- ✅ Web interface accessible
- ✅ No new watchdog timeouts
- ✅ Memory usage stable

---

## Future Enhancements

### Post-v0.4.0 Ideas

**1. Preset Layouts**
- Bundle INI files: `compact.ini`, `large_fonts.ini`, `high_contrast.ini`
- Download from web UI dropdown
- Community-shared layouts

**2. Color Overrides**
- Allow RGB565 color customization
- Theme system (dark mode, high contrast, etc.)

**3. Element Visibility Toggle**
- Hide/show specific sections
- Useful for minimal displays

**4. Export/Import**
- Download INI from web UI
- Upload custom INI files
- Share layouts between devices

**5. Live Preview on LCD**
- "Preview Mode" that shows changes on device screen
- Toggle between preview and current layout
- Requires more complex state management

**6. Touch-Based Adjustment**
- Long-press LCD element → edit mode
- Drag to reposition
- Pinch to resize fonts
- Requires significant touchscreen work

---

## Implementation Checklist

### Phase 0: Proof of Concept
- [ ] Create `src/display/layout_config.h`
- [ ] Create `src/display/layout_config.cpp`
- [ ] Implement MonitorLayoutConfig struct (Header section only)
- [ ] Implement basic INI parser
- [ ] Modify `ui_monitor.cpp` to use runtime values
- [ ] Test with manual INI file
- [ ] Verify boot safety
- [ ] **STOP POINT: Validate approach**

### Phase 1: Backend Foundation
- [ ] Expand MonitorLayoutConfig (all sections)
- [ ] Complete INI parser (all sections, error handling)
- [ ] Add `GET /api/layout/defaults`
- [ ] Add `GET /api/layout/overrides/:screen`
- [ ] Add `POST /api/layout/save`
- [ ] Add `POST /api/layout/reload`
- [ ] Update all Monitor drawing code
- [ ] Test with curl
- [ ] **STOP POINT: Backend complete**

### Phase 2: Simple Settings Form
- [ ] Add layout section to `settings.html`
- [ ] Add JavaScript handlers
- [ ] Load current values
- [ ] Save/reload functionality
- [ ] Reset to defaults
- [ ] Test user flow
- [ ] **STOP POINT: Functional release**

### Phase 3: Visual Editor
- [ ] Adapt `editor.html` structure
- [ ] Implement JavaScript canvas rendering
- [ ] Add override comparison UI
- [ ] Implement live preview
- [ ] Add section highlighting
- [ ] Test visual editor
- [ ] **STOP POINT: Polished release**

### Phase 4: Expand to Other Screens
- [ ] Alignment mode support
- [ ] Graph mode support
- [ ] Network mode support
- [ ] Storage mode support
- [ ] Test all modes
- [ ] **COMPLETE: v0.4.0 release**

---

## Success Criteria

**Phase 0 Success:**
- ✅ Concept proven with minimal code
- ✅ No boot issues with missing/corrupt INI
- ✅ Header overrides work visually

**Phase 1 Success:**
- ✅ All Monitor sections overridable
- ✅ API endpoints functional
- ✅ Reload works without reboot

**Phase 2 Success:**
- ✅ Users can adjust layouts via web form
- ✅ Changes visible immediately

**Phase 3 Success:**
- ✅ Visual editor provides instant preview
- ✅ Editing experience is intuitive
- ✅ Canvas matches LCD output

**Overall Success:**
- ✅ Users can customize layouts without code changes
- ✅ System is boot-safe and reliable
- ✅ Both simple and advanced workflows available
- ✅ Ready for v0.4.0 release

---

## Notes for Implementation Session

### Prerequisites
- FluidDash v0.3.100 stable and tested
- Current branch: `main` (or create `feature/ini-layout-overrides`)
- Hardware available for testing (optional but recommended)

### Recommended Approach
1. **Start Phase 0 only** - prove the concept works
2. **Get user feedback** - is this the right direction?
3. **Continue phases** if concept validated
4. **Stop at Phase 2** for quick release, Phase 3 optional

### Time Estimates
- Phase 0: 1-2 hours
- Phase 1: 2-3 hours
- Phase 2: 1 hour
- Phase 3: 3-4 hours
- Phase 4: 4 hours (1 hour × 4 screens)
- **Total: 11-14 hours** (can be done across multiple sessions)

### Risk Mitigation
- All phases are independent - can stop anytime
- INI files are optional - no risk to existing functionality
- Hard-coded defaults always work
- Easy to rollback (delete INI file)

---

**End of Implementation Guide**

**Status:** Ready for implementation
**Next Session:** Start with Phase 0 proof of concept
**Estimated Completion:** v0.4.0 (2-3 implementation sessions)
