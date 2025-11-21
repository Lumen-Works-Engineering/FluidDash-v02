#include "touch_handler.h"
#include "display/display.h"
#include "display/ui_modes.h"
#include "state/global_state.h"

// External variables from main.cpp
extern DisplayMode currentMode;
extern void enterSetupMode();  // Function to enter WiFi AP mode

static unsigned long lastTouchTime = 0;
static unsigned long headerHoldStartTime = 0;
static bool isHoldingHeader = false;

void handleTouchInput() {
    uint16_t x, y;
    unsigned long now = millis();

    // Check if screen is touched
    if (gfx.getTouch(&x, &y)) {
        // Detect touch zones based on Y coordinate
        if (y < TOUCH_ZONE_HEADER_Y_MAX) {
            // Header zone - require 5 second hold
            if (!isHoldingHeader) {
                // Start of hold
                isHoldingHeader = true;
                headerHoldStartTime = now;
                Serial.println("[TOUCH] Header hold started - hold for 5s to enter WiFi setup");
            } else {
                // Continue holding - check duration and update progress
                unsigned long holdDuration = now - headerHoldStartTime;
                int progress = (holdDuration * 100) / TOUCH_HOLD_DURATION_MS;

                if (progress <= 100) {
                    drawProgressBar(progress);
                }

                // Check if hold completed
                if (holdDuration >= TOUCH_HOLD_DURATION_MS) {
                    Serial.println("[TOUCH] Header hold complete - entering setup mode");
                    isHoldingHeader = false;
                    enterSetupMode();
                    drawScreen();  // Redraw after returning from setup
                }
            }
        } else if (y > TOUCH_ZONE_FOOTER_Y_MIN) {
            // Footer zone - cycle screens (with debounce)
            if (now - lastTouchTime >= TOUCH_DEBOUNCE_MS) {
                lastTouchTime = now;
                Serial.println("[TOUCH] Footer zone tapped - cycling mode");
                cycleModeForward();
                drawScreen();
            }
            // Cancel header hold if we move to footer
            if (isHoldingHeader) {
                isHoldingHeader = false;
                Serial.println("[TOUCH] Header hold cancelled");
                drawScreen();  // Clear progress bar
            }
        } else {
            // Middle zone - cancel header hold if active
            if (isHoldingHeader) {
                isHoldingHeader = false;
                Serial.println("[TOUCH] Header hold cancelled - moved to middle zone");
                drawScreen();  // Clear progress bar
            }
        }
    } else {
        // No touch detected - cancel header hold if active
        if (isHoldingHeader) {
            isHoldingHeader = false;
            Serial.println("[TOUCH] Header hold cancelled - touch released");
            drawScreen();  // Clear progress bar
        }
    }
}

void cycleModeForward() {
    // Cycle through 5 display modes: Monitor -> Alignment -> Graph -> Network -> Storage -> Monitor
    currentMode = (DisplayMode)((currentMode + 1) % 5);
}

void drawProgressBar(int progress) {
    // Draw a progress bar at the top of the screen
    // Progress is 0-100
    if (progress < 0) progress = 0;
    if (progress > 100) progress = 100;

    const int barX = 10;
    const int barY = 5;
    const int barWidth = 300;
    const int barHeight = 10;

    // Draw background (dark gray)
    gfx.fillRect(barX, barY, barWidth, barHeight, 0x31A6);  // Dark gray

    // Draw progress (cyan)
    int fillWidth = (barWidth * progress) / 100;
    gfx.fillRect(barX, barY, fillWidth, barHeight, 0x07FF);  // Cyan

    // Draw border (white)
    gfx.drawRect(barX, barY, barWidth, barHeight, 0xFFFF);  // White

    // Draw text
    gfx.setTextColor(0xFFFF, 0x0000);  // White on black
    gfx.setTextSize(1);
    gfx.setCursor(320, 7);
    gfx.printf("%d%%", progress);
}
