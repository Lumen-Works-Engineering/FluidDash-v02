#include "ui_modes.h"
#include "display.h"
#include "config/config.h"

// External variables from main.cpp
extern Config cfg;
extern DisplayMode currentMode;

// ========== MAIN DISPLAY CONTROL ==========
void drawScreen() {
    switch(currentMode) {
        case MODE_MONITOR:
            drawMonitorMode();
            break;
        case MODE_ALIGNMENT:
            drawAlignmentMode();
            break;
        case MODE_GRAPH:
            drawGraphMode();
            break;
        case MODE_NETWORK:
            drawNetworkMode();
            break;
    }
}

void updateDisplay() {
    switch(currentMode) {
        case MODE_MONITOR:
            updateMonitorMode();
            break;
        case MODE_ALIGNMENT:
            updateAlignmentMode();
            break;
        case MODE_GRAPH:
            updateGraphMode();
            break;
        case MODE_NETWORK:
            updateNetworkMode();
            break;
    }
}
