# Layout Code Generator - Development Tool

**Created:** 2025-01-22
**Status:** Design Complete - Ready for Implementation
**Purpose:** WYSIWYG development tool for hard-coded layout constants
**Estimated Effort:** 3-4 hours total

---

## Table of Contents

1. [Concept Overview](#concept-overview)
2. [Why This Approach](#why-this-approach)
3. [How It Works](#how-it-works)
4. [Implementation Plan](#implementation-plan)
5. [Technical Specifications](#technical-specifications)
6. [UI Design](#ui-design)
7. [Code Examples](#code-examples)
8. [Testing Plan](#testing-plan)

---

## Concept Overview

### The Problem

Adjusting hard-coded layout constants is tedious:

**Current workflow:**
1. Edit `ui_layout.h` → change `TEMP_LABEL_X = 10` to `= 25`
2. Recompile firmware
3. Upload to device
4. Look at screen - "hmm, not quite right"
5. Edit again → `= 30`
6. Recompile, upload, check
7. **Repeat 10-20 times** to get it perfect

**Result:** Hours of compile-test-repeat cycles for simple positioning adjustments.

### The Solution

A **visual code generator** that runs in the browser:

1. **Design visually** - drag, resize, adjust values with sliders
2. **See instant preview** - canvas shows exactly what LCD will display
3. **Iterate quickly** - try 20 variations in 5 minutes
4. **Generate C++ code** - click button, get ready-to-paste constants
5. **Copy/paste once** - update `ui_layout.h` with final values
6. **Compile once** - done!

**Result:** Minutes of visual iteration, ONE compile cycle.

---

## Why This Approach

### Advantages Over Runtime Configuration (INI/JSON)

| Aspect | Runtime Config (INI) | Code Generator (This) |
|--------|----------------------|------------------------|
| **Complexity** | High (parser, loader, API) | Low (standalone HTML) |
| **Boot risk** | Medium (file I/O, parsing) | Zero (dev tool only) |
| **Memory overhead** | ~5-10KB (runtime structs) | Zero (hard-coded) |
| **Performance** | Minimal overhead | Zero overhead |
| **Type safety** | Runtime errors possible | Compile-time validated |
| **Version control** | Config files separate | Code changes in git |
| **User workflow** | Edit INI, reload device | Edit visually, paste code |
| **Maintenance** | Backend + frontend | Frontend only |
| **Use case** | Frequent layout changes | Initial setup + rare tweaks |

### Perfect For Your Use Case

> "Knowing me it will take a while to get the screens just like I want them but after that they will be static."

✅ **Initial design phase:** Visual iteration is WAY faster
✅ **Post-design phase:** Hard-coded = zero runtime complexity
✅ **Future changes:** Rare enough that recompile is acceptable
✅ **Development tool:** Other developers can use it too

---

## How It Works

### Workflow

```
┌─────────────────────────────────────────────────────────┐
│  1. LOAD CURRENT VALUES                                 │
│     Option A: Paste current ui_layout.h constants       │
│     Option B: Fetch from /api/layout/source (optional)  │
└─────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  2. VISUAL EDITING                                      │
│     • Canvas shows 480×320 preview                      │
│     • Click sections to select                          │
│     • Adjust values with sliders/inputs                 │
│     • See changes instantly on canvas                   │
│     • Try different fonts, positions, spacing           │
└─────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  3. GENERATE CODE                                       │
│     • Click "Generate Code" button                      │
│     • C++ namespace with all constants                  │
│     • Comments show modified values                     │
│     • Copy to clipboard                                 │
└─────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  4. UPDATE SOURCE CODE                                  │
│     • Paste into src/display/ui_layout.h                │
│     • Replace MonitorLayout namespace                   │
│     • Commit to git                                     │
└─────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  5. COMPILE & UPLOAD                                    │
│     • pio run -e esp32dev                               │
│     • pio run -e esp32dev -t upload                     │
│     • Perfect layout on first try!                      │
└─────────────────────────────────────────────────────────┘
```

### Example Use Case

**Goal:** Adjust Monitor screen temperature display

**With code generator (5 minutes):**
1. Open `layout_generator.html` in browser
2. Load current Monitor layout
3. Select "Temperature" section
4. Try different values:
   - Label X: 10 → 15 → 20 → 18 (perfect!)
   - Value X: 120 → 130 → 125 (perfect!)
   - Row spacing: 30 → 35 → 32 (perfect!)
   - Font size: 2 → 3 → 2 (leave at 2)
5. Generate code, copy to clipboard
6. Paste into `ui_layout.h`
7. Compile, upload - done!

**Without code generator (30+ minutes):**
1. Edit `ui_layout.h` → change value
2. Compile (30 seconds)
3. Upload (10 seconds)
4. Check screen - not quite right
5. Repeat 15 times...

---

## Implementation Plan

### Phase 1: Core Code Generator (2-3 hours)

**Goal:** Standalone HTML tool that generates C++ code

**Tasks:**

1. **Adapt existing editor.html**
   - Remove element creation tools (rect, line drawing)
   - Add layout section selector (Header, Temperature, Position, etc.)
   - Modify properties panel for constant editing
   - Keep canvas preview system

2. **Implement JavaScript canvas rendering**
   - Create `renderMonitorScreen(layout)` function
   - Match LCD output as closely as possible
   - Render all sections with current values

3. **Add code generation**
   - `generateCppCode()` function
   - Format as proper C++ namespace
   - Add comments for modified values
   - Display in textarea for copy/paste

4. **Add value loading**
   - Manual paste option (paste current constants)
   - Or simple input form to enter current values
   - Parse into layout object

**Files:**
- `data/web/layout_generator.html` (new, adapted from editor.html)

**No backend changes needed!** Pure client-side tool.

**Success Criteria:**
- ✅ Can load current layout values
- ✅ Canvas preview shows accurate representation
- ✅ Value changes update canvas instantly
- ✅ Generate button produces valid C++ code
- ✅ Generated code is copy/paste ready

---

### Phase 2: Optional Backend Integration (1 hour)

**Goal:** Convenience endpoint to load current values automatically

**Tasks:**

1. **Add source code endpoint**
   - `GET /api/layout/source?screen=monitor`
   - Returns current ui_layout.h constants as JSON
   - Generator can load with one click

2. **Serve generator from device**
   - Add endpoint: `GET /layout-generator`
   - Serve layout_generator.html
   - Access from any browser on network

**Files Modified:**
- `src/web/web_handlers.cpp` (add 2 endpoints)

**Success Criteria:**
- ✅ "Load Current Values" button works
- ✅ No need to manually paste constants
- ✅ Can access generator from device IP

**Note:** This phase is **completely optional** - the tool works standalone without it.

---

## Technical Specifications

### Data Structure

The tool works with a simple JavaScript object representing layout constants:

```javascript
const monitorLayout = {
    Header: {
        textX: 10,
        textY: 6,
        fontSize: 2,
        datetimeX: 270,
        datetimeY: 6
    },

    Temperature: {
        sectionX: 10,
        labelY: 30,
        labelFontSize: 1,
        startY: 50,
        rowSpacing: 30,
        valueX: 50,
        valueYOffset: -3,
        valueFontSize: 2,
        peakTempX: 140,
        peakTempYOffset: 2,
        peakTempFontSize: 1
    },

    Position: {
        statusY: 185,
        statusFontSize: 2,
        statusLabelX: 10,
        statusValueX: 90,
        coordRow1Y: 210,
        coordRow2Y: 240,
        coordRow3Y: 270,
        coordFontSize: 2
    },

    PSU: {
        labelY: 30,
        valueY: 50,
        fontSize: 3,
        sectionX: 250
    },

    Fan: {
        labelY: 105,
        rpmY: 125,
        speedY: 155,
        valueFontSize: 2,
        sectionX: 250
    },

    Dividers: {
        topY: 25,
        middleY: 175,
        verticalX: 240
    }
};
```

### Code Generation Algorithm

```javascript
function generateCppCode(layout, defaults) {
    let code = '';

    // Header comment
    code += '// ========== MONITOR MODE LAYOUT ==========\n';
    code += '// Generated by FluidDash Layout Code Generator\n';
    code += `// Date: ${new Date().toISOString()}\n`;
    code += '// NOTE: Replace the MonitorLayout namespace in ui_layout.h\n\n';

    // Namespace
    code += 'namespace MonitorLayout {\n';

    // Iterate sections
    for (let section in layout) {
        code += `    // ${section}\n`;

        for (let key in layout[section]) {
            const value = layout[section][key];
            const defaultValue = defaults[section][key];
            const constName = toConstantName(key);

            // Add comment if value was modified
            let comment = '';
            if (value !== defaultValue) {
                comment = `  // Modified (was ${defaultValue})`;
            }

            code += `    constexpr int ${constName} = ${value};${comment}\n`;
        }

        code += '\n';
    }

    code += '};\n';

    return code;
}

// Convert camelCase to SCREAMING_SNAKE_CASE
function toConstantName(key) {
    return key
        .replace(/([A-Z])/g, '_$1')
        .toUpperCase();
}
```

**Example Output:**

```cpp
// ========== MONITOR MODE LAYOUT ==========
// Generated by FluidDash Layout Code Generator
// Date: 2025-01-22T14:30:00.000Z
// NOTE: Replace the MonitorLayout namespace in ui_layout.h

namespace MonitorLayout {
    // Header
    constexpr int TEXT_X = 10;
    constexpr int TEXT_Y = 15;  // Modified (was 6)
    constexpr int FONT_SIZE = 3;  // Modified (was 2)
    constexpr int DATETIME_X = 270;
    constexpr int DATETIME_Y = 6;

    // Temperature
    constexpr int SECTION_X = 10;
    constexpr int LABEL_Y = 30;
    constexpr int LABEL_FONT_SIZE = 1;
    constexpr int START_Y = 50;
    constexpr int ROW_SPACING = 35;  // Modified (was 30)
    constexpr int VALUE_X = 120;
    constexpr int VALUE_Y_OFFSET = -3;
    constexpr int VALUE_FONT_SIZE = 2;

    // ... rest of constants
};
```

### Canvas Rendering

The tool renders a JavaScript preview that matches the LCD output:

```javascript
function renderMonitorScreen(layout) {
    const ctx = canvas.getContext('2d');

    // Clear canvas
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, 480, 320);

    // === HEADER ===
    ctx.fillStyle = '#FFF';
    ctx.font = `${layout.Header.fontSize * 8}px monospace`;
    ctx.fillText('FluidDash', layout.Header.textX, layout.Header.textY + 15);

    ctx.fillText('2025-01-22 14:30', layout.Header.datetimeX, layout.Header.datetimeY + 15);

    // === DIVIDERS ===
    ctx.strokeStyle = '#888';
    ctx.lineWidth = 1;

    // Top divider
    ctx.beginPath();
    ctx.moveTo(0, layout.Dividers.topY);
    ctx.lineTo(480, layout.Dividers.topY);
    ctx.stroke();

    // Vertical divider
    ctx.beginPath();
    ctx.moveTo(layout.Dividers.verticalX, layout.Dividers.topY);
    ctx.lineTo(layout.Dividers.verticalX, layout.Dividers.middleY);
    ctx.stroke();

    // Middle divider
    ctx.beginPath();
    ctx.moveTo(0, layout.Dividers.middleY);
    ctx.lineTo(480, layout.Dividers.middleY);
    ctx.stroke();

    // === TEMPERATURE LABELS ===
    const tempLabels = ['X-Axis:', 'Y-Left:', 'Y-Right:', 'Z-Axis:'];
    const tempValues = [26.5, 26.3, 26.4, 26.2];
    const peakValues = [28.1, 27.9, 28.0, 27.8];

    tempLabels.forEach((label, i) => {
        const y = layout.Temperature.startY + (i * layout.Temperature.rowSpacing);

        // Label
        ctx.fillStyle = '#FFF';
        ctx.font = `${layout.Temperature.labelFontSize * 8}px monospace`;
        ctx.fillText(label, layout.Temperature.sectionX, y);

        // Value
        ctx.fillStyle = '#0F0';
        ctx.font = `${layout.Temperature.valueFontSize * 8}px monospace`;
        ctx.fillText(`${tempValues[i].toFixed(1)}°C`,
                     layout.Temperature.valueX,
                     y + layout.Temperature.valueYOffset);

        // Peak
        ctx.fillStyle = '#FF0';
        ctx.font = `${layout.Temperature.peakTempFontSize * 8}px monospace`;
        ctx.fillText(`(${peakValues[i].toFixed(1)}°C)`,
                     layout.Temperature.peakTempX,
                     y + layout.Temperature.peakTempYOffset);
    });

    // === PSU VOLTAGE ===
    ctx.fillStyle = '#FFF';
    ctx.font = `${layout.PSU.fontSize * 8}px monospace`;
    ctx.fillText('PSU', layout.PSU.sectionX, layout.PSU.labelY);

    ctx.fillStyle = '#0FF';
    ctx.fillText('12.1V', layout.PSU.sectionX, layout.PSU.valueY);

    // === FAN ===
    ctx.fillStyle = '#FFF';
    ctx.font = `${layout.Fan.valueFontSize * 8}px monospace`;
    ctx.fillText('Fan', layout.Fan.sectionX, layout.Fan.labelY);
    ctx.fillText('2450 RPM', layout.Fan.sectionX, layout.Fan.rpmY);
    ctx.fillText('50%', layout.Fan.sectionX, layout.Fan.speedY);

    // === POSITION (if FluidNC connected) ===
    ctx.fillStyle = '#FFF';
    ctx.font = `${layout.Position.statusFontSize * 8}px monospace`;
    ctx.fillText('State:', layout.Position.statusLabelX, layout.Position.statusY);
    ctx.fillStyle = '#0F0';
    ctx.fillText('Idle', layout.Position.statusValueX, layout.Position.statusY);

    // Coordinates
    ctx.fillStyle = '#FFF';
    ctx.font = `${layout.Position.coordFontSize * 8}px monospace`;
    ctx.fillText('X: 10.234  Y: 5.123', layout.Position.statusLabelX, layout.Position.coordRow1Y);
    ctx.fillText('Z: 0.500   A: 0.000', layout.Position.statusLabelX, layout.Position.coordRow2Y);
    ctx.fillText('F: 1500', layout.Position.statusLabelX, layout.Position.coordRow3Y);
}
```

---

## UI Design

### Layout

```
┌──────────────────────────────────────────────────────────────────┐
│ FluidDash Layout Code Generator                                  │
│ ────────────────────────────────────────────────────────────────│
│                                                                   │
│ Screen Mode: [Monitor ▼]  [Load Values] [Load from Device]      │
│                                                                   │
│ ┌────────────────────┐  ┌──────────────────────────────────────┐│
│ │  Sections          │  │  Canvas Preview (480×320)             ││
│ │  ────────────────  │  │                                       ││
│ │  ✓ Header          │  │  ┌──────────────────────────────────┐││
│ │  ⚠️ Temperature     │  │  │                                  │││
│ │  ✓ Position        │  │  │   [Monitor screen rendered       │││
│ │  ✓ PSU             │  │  │    with current values]          │││
│ │  ✓ Fan             │  │  │                                  │││
│ │  ✓ Dividers        │  │  │                                  │││
│ │                    │  │  │                                  │││
│ │  ⚠️ = modified      │  │  └──────────────────────────────────┘││
│ └────────────────────┘  └──────────────────────────────────────┘│
│                                                                   │
│ ┌──────────────────────────────────────────────────────────────┐│
│ │ Properties: Temperature                                       ││
│ │ ────────────────────────────────────────────────────────────│││
│ │                                                               ││
│ │ Section X Position:  [____10____]  (Default: 10)             ││
│ │ Label Y Position:    [____30____]  (Default: 30)             ││
│ │ Label Font Size:     [____1_____]  (Default: 1)              ││
│ │ Row Spacing:         [____35____]  ⚠️ Default: 30             ││
│ │ Value X Position:    [___120____]  (Default: 120)            ││
│ │ Value Font Size:     [____2_____]  (Default: 2)              ││
│ │                                                               ││
│ │ [Reset Section to Defaults]                                  ││
│ └──────────────────────────────────────────────────────────────┘│
│                                                                   │
│ ┌──────────────────────────────────────────────────────────────┐│
│ │ Generated C++ Code                         [Copy to Clipboard]││
│ │ ────────────────────────────────────────────────────────────│││
│ │                                                               ││
│ │ namespace MonitorLayout {                                     ││
│ │     // Header                                                 ││
│ │     constexpr int TEXT_X = 10;                                ││
│ │     constexpr int TEXT_Y = 6;                                 ││
│ │     constexpr int FONT_SIZE = 2;                              ││
│ │                                                               ││
│ │     // Temperature                                            ││
│ │     constexpr int ROW_SPACING = 35;  // Modified (was 30)    ││
│ │     ...                                                       ││
│ │ };                                                            ││
│ └──────────────────────────────────────────────────────────────┘│
│                                                                   │
│ [Generate Code] [Reset All to Defaults] [Export JSON]           │
└──────────────────────────────────────────────────────────────────┘
```

### Interaction Flow

**1. Load Values**
- Click "Load Values" → Paste dialog opens
- Paste current MonitorLayout constants
- Or click "Load from Device" (if backend implemented)

**2. Select Section**
- Click section name in left panel
- Canvas highlights that section
- Properties panel shows all constants for section

**3. Adjust Values**
- Use number inputs or sliders
- Canvas updates in real-time
- Modified values show ⚠️ indicator

**4. Generate Code**
- Click "Generate Code" button
- C++ code appears in bottom textarea
- Click "Copy to Clipboard"
- Paste into ui_layout.h

---

## Code Examples

### Main HTML Structure

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>FluidDash Layout Code Generator</title>
    <style>
        /* Reuse styling from editor.html */
        /* ... */
    </style>
</head>
<body>
    <header>
        <h1>🎨 FluidDash Layout Code Generator</h1>
        <div class="subtitle">Visual Development Tool for Hard-Coded Layout Constants</div>
    </header>

    <div class="controls">
        <select id="screenMode">
            <option value="monitor">Monitor Mode</option>
            <option value="alignment">Alignment Mode</option>
            <option value="graph">Graph Mode</option>
            <option value="network">Network Mode</option>
            <option value="storage">Storage Mode</option>
        </select>

        <button onclick="loadValuesManual()">📋 Load Values (Paste)</button>
        <button onclick="loadValuesFromDevice()">📡 Load from Device</button>
        <button onclick="resetAll()">🔄 Reset All</button>
    </div>

    <div class="container">
        <div class="sections-panel">
            <h3>Sections</h3>
            <div id="sectionsList"></div>
        </div>

        <div class="canvas-panel">
            <canvas id="canvas" width="480" height="320"></canvas>
        </div>

        <div class="properties-panel">
            <h3 id="propertiesTitle">Properties</h3>
            <div id="propertiesContent"></div>
        </div>
    </div>

    <div class="code-panel">
        <h3>Generated C++ Code
            <button onclick="copyCode()">📋 Copy to Clipboard</button>
        </h3>
        <textarea id="generatedCode" readonly rows="20"></textarea>
    </div>

    <script src="layout_generator.js"></script>
</body>
</html>
```

### Core JavaScript Functions

```javascript
// State
let currentMode = 'monitor';
let currentLayout = {};
let defaultLayout = {};
let selectedSection = null;

// Load values manually (paste)
function loadValuesManual() {
    const input = prompt('Paste MonitorLayout namespace constants:');
    if (!input) return;

    // Parse pasted C++ code
    currentLayout = parseCppConstants(input);
    defaultLayout = { ...currentLayout }; // Store as defaults

    updateUI();
}

// Load from device (optional)
async function loadValuesFromDevice() {
    try {
        const response = await fetch(`/api/layout/source?screen=${currentMode}`);
        const data = await response.json();

        currentLayout = data;
        defaultLayout = { ...data };

        updateUI();
    } catch (error) {
        alert('Could not load from device. Use manual paste instead.');
    }
}

// Parse C++ constants
function parseCppConstants(cppCode) {
    const layout = {};
    let currentSection = null;

    // Match: constexpr int SOME_VALUE = 123;
    const constPattern = /constexpr\s+int\s+(\w+)\s*=\s*(\d+)/g;

    // Match section comments: // Header
    const sectionPattern = /\/\/\s*(\w+)/g;

    const lines = cppCode.split('\n');

    lines.forEach(line => {
        // Check for section comment
        const sectionMatch = line.match(/\/\/\s*(\w+)/);
        if (sectionMatch) {
            currentSection = sectionMatch[1];
            layout[currentSection] = {};
            return;
        }

        // Check for constant
        const constMatch = line.match(/constexpr\s+int\s+(\w+)\s*=\s*(-?\d+)/);
        if (constMatch && currentSection) {
            const [, name, value] = constMatch;
            const key = toCamelCase(name);
            layout[currentSection][key] = parseInt(value);
        }
    });

    return layout;
}

// Convert SCREAMING_SNAKE_CASE to camelCase
function toCamelCase(str) {
    return str.toLowerCase().replace(/_([a-z])/g, (g) => g[1].toUpperCase());
}

// Update all UI elements
function updateUI() {
    updateSectionsList();
    updateCanvas();
    updateProperties();
    generateCode();
}

// Update sections list
function updateSectionsList() {
    const list = document.getElementById('sectionsList');
    list.innerHTML = '';

    for (let section in currentLayout) {
        const modified = isModified(section);
        const indicator = modified ? '⚠️' : '✓';

        const div = document.createElement('div');
        div.className = 'section-item';
        div.innerHTML = `${indicator} ${section}`;
        div.onclick = () => selectSection(section);
        list.appendChild(div);
    }
}

// Check if section has modifications
function isModified(section) {
    for (let key in currentLayout[section]) {
        if (currentLayout[section][key] !== defaultLayout[section][key]) {
            return true;
        }
    }
    return false;
}

// Select section
function selectSection(section) {
    selectedSection = section;
    updateProperties();
}

// Update properties panel
function updateProperties() {
    if (!selectedSection) {
        document.getElementById('propertiesContent').innerHTML =
            '<p>Select a section</p>';
        return;
    }

    const section = currentLayout[selectedSection];
    const defaults = defaultLayout[selectedSection];

    let html = '';

    for (let key in section) {
        const value = section[key];
        const defaultValue = defaults[key];
        const modified = value !== defaultValue;
        const indicator = modified ?
            `<span class="modified">⚠️ Default: ${defaultValue}</span>` :
            `<span class="default">Default: ${defaultValue}</span>`;

        html += `
            <div class="property-group">
                <label>${toHumanReadable(key)}</label>
                <input type="number"
                       value="${value}"
                       onchange="updateValue('${selectedSection}', '${key}', parseInt(this.value))">
                ${indicator}
            </div>
        `;
    }

    html += `<button onclick="resetSection('${selectedSection}')">Reset Section</button>`;

    document.getElementById('propertiesContent').innerHTML = html;
    document.getElementById('propertiesTitle').textContent =
        `Properties: ${selectedSection}`;
}

// Convert camelCase to Human Readable
function toHumanReadable(str) {
    return str
        .replace(/([A-Z])/g, ' $1')
        .replace(/^./, (s) => s.toUpperCase());
}

// Update value
function updateValue(section, key, value) {
    currentLayout[section][key] = value;
    updateCanvas();
    updateSectionsList();
    generateCode();
}

// Reset section
function resetSection(section) {
    currentLayout[section] = { ...defaultLayout[section] };
    updateUI();
}

// Reset all
function resetAll() {
    if (!confirm('Reset all values to defaults?')) return;
    currentLayout = { ...defaultLayout };
    updateUI();
}

// Generate C++ code
function generateCode() {
    const textarea = document.getElementById('generatedCode');
    textarea.value = generateCppCode(currentLayout, defaultLayout);
}

// Copy code to clipboard
function copyCode() {
    const textarea = document.getElementById('generatedCode');
    textarea.select();
    document.execCommand('copy');
    alert('Code copied to clipboard!');
}
```

---

## Testing Plan

### Manual Testing Checklist

**Phase 1: Core Functionality**
- [ ] Load values by pasting C++ code
- [ ] Parse constants correctly
- [ ] Section list displays all sections
- [ ] Select section updates properties panel
- [ ] Modify value updates canvas
- [ ] Canvas preview matches expectations
- [ ] Generate button produces valid C++ code
- [ ] Copy to clipboard works
- [ ] Reset section works
- [ ] Reset all works
- [ ] Modified indicators show correctly

**Phase 2: Backend Integration (Optional)**
- [ ] "Load from Device" button works
- [ ] API endpoint returns correct JSON
- [ ] Values load automatically
- [ ] Generator accessible from device IP

### Validation Tests

**Generated Code Quality:**
- [ ] Valid C++ syntax
- [ ] Proper namespace structure
- [ ] Correct constant naming (SCREAMING_SNAKE_CASE)
- [ ] Comments on modified values accurate
- [ ] Compiles without errors when pasted into ui_layout.h
- [ ] Produces identical LCD output to preview

**Canvas Rendering Accuracy:**
- [ ] Fonts match LCD output
- [ ] Positions match LCD output
- [ ] Colors close approximation
- [ ] All sections rendered
- [ ] Updates in real-time

---

## Implementation Checklist

### Phase 1: Core Generator
- [ ] Adapt editor.html to layout_generator.html
- [ ] Remove element creation UI
- [ ] Add section selector panel
- [ ] Implement canvas rendering for Monitor mode
- [ ] Add properties panel for constant editing
- [ ] Implement C++ code generation
- [ ] Add manual value loading (paste)
- [ ] Test with current ui_layout.h constants
- [ ] Verify generated code compiles
- [ ] **STOP: Core tool functional**

### Phase 2: Backend Integration (Optional)
- [ ] Add GET /api/layout/source endpoint
- [ ] Return current constants as JSON
- [ ] Add GET /layout-generator endpoint
- [ ] Serve HTML from device
- [ ] Add "Load from Device" button to UI
- [ ] Test from browser on network
- [ ] **COMPLETE: Polished tool**

---

## Success Criteria

**Phase 1 Success:**
- ✅ Can load current layout constants
- ✅ Can adjust values visually
- ✅ Canvas shows accurate preview
- ✅ Generated code is valid C++
- ✅ Code compiles without errors
- ✅ LCD output matches preview

**Overall Success:**
- ✅ Layout iteration is 10x faster than edit-compile-test
- ✅ Tool is easy to use for any developer
- ✅ No runtime complexity added to firmware
- ✅ Hard-coded architecture preserved
- ✅ Ready for use in screen design phase

---

## Future Enhancements

### Post-v1.0 Ideas

**1. All Screen Modes**
- Implement canvas rendering for Alignment, Graph, Network, Storage modes
- Same workflow for all screens

**2. Presets**
- Save/load preset layouts
- "Compact", "Large Fonts", "High Contrast" presets
- Export/import JSON files

**3. Grid Snapping**
- Optional grid overlay on canvas
- Snap values to 5px increments
- Alignment guides

**4. Undo/Redo**
- History stack for value changes
- Ctrl+Z / Ctrl+Y keyboard shortcuts

**5. Live Device Preview**
- WebSocket connection to device
- Send values to device for live preview
- See changes on actual LCD in real-time

**6. Batch Operations**
- Shift all values in section by N pixels
- Scale all font sizes by percentage
- Copy section from one mode to another

---

## Notes for Implementation

### Prerequisites
- Existing editor.html as starting point
- Understanding of current ui_layout.h structure
- Browser with HTML5 canvas support

### Recommended Approach
1. Start with Monitor mode only
2. Perfect the canvas rendering
3. Test code generation thoroughly
4. Expand to other modes after validation

### Time Estimate
- Phase 1 (core tool): 2-3 hours
- Phase 2 (backend): 1 hour
- Testing: 30 minutes
- **Total: 3-4 hours**

### Success Indicators
- ✅ First compile after using tool succeeds
- ✅ LCD output matches preview exactly
- ✅ Tool saves hours on layout design
- ✅ Other developers can use it easily

---

**End of Layout Code Generator Specification**

**Status:** Ready for implementation
**Priority:** High (saves significant development time)
**Complexity:** Low-Medium (mostly UI work)
**Value:** Very High (accelerates layout design)
