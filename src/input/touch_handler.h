#ifndef TOUCH_HANDLER_H
#define TOUCH_HANDLER_H

#include <Arduino.h>

// Touch zone definitions (Y coordinates)
#define TOUCH_ZONE_HEADER_Y_MAX 25      // Tap header = WiFi setup (with 5s hold)
#define TOUCH_ZONE_FOOTER_Y_MIN 280     // Tap footer = cycle screens
#define TOUCH_DEBOUNCE_MS 300           // 300ms debounce for footer tap
#define TOUCH_HOLD_DURATION_MS 5000     // 5 seconds hold required for WiFi setup

// Touch handler functions
void handleTouchInput();
void cycleModeForward();
void drawProgressBar(int progress);  // Draw hold progress indicator

#endif // TOUCH_HANDLER_H
