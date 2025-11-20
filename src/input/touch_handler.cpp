#include "touch_handler.h"
#include "display/display.h"
#include "display/ui_modes.h"
#include "state/global_state.h"

// External variables from main.cpp
extern DisplayMode currentMode;
extern void enterSetupMode();  // Function to enter WiFi AP mode

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

        // Debug: Print touch coordinates
        Serial.print("[TOUCH] Detected at X=");
        Serial.print(x);
        Serial.print(", Y=");
        Serial.println(y);

        // Detect touch zones based on Y coordinate
        if (y < TOUCH_ZONE_HEADER_Y_MAX) {
            // Header tap - enter WiFi setup mode
            Serial.println("[TOUCH] Header zone tapped - entering setup mode");
            enterSetupMode();
        } else if (y > TOUCH_ZONE_FOOTER_Y_MIN) {
            // Footer tap - cycle to next screen
            Serial.println("[TOUCH] Footer zone tapped - cycling mode");
            cycleModeForward();
            drawScreen();
        } else {
            // Middle area - no action
            Serial.println("[TOUCH] Middle zone tapped - no action");
        }
    }
}

void cycleModeForward() {
    // Cycle through 4 display modes: Monitor -> Alignment -> Graph -> Network -> Monitor
    currentMode = (DisplayMode)((currentMode + 1) % 4);
}
