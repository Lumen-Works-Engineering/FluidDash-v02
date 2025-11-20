#ifndef TOUCH_HANDLER_H
#define TOUCH_HANDLER_H

#include <Arduino.h>

// Touch zone definitions (Y coordinates)
#define TOUCH_ZONE_HEADER_Y_MAX 25      // Tap header = WiFi setup
#define TOUCH_ZONE_FOOTER_Y_MIN 280     // Tap footer = cycle screens
#define TOUCH_DEBOUNCE_MS 300           // 300ms debounce

// Touch handler functions
void handleTouchInput();
void cycleModeForward();

#endif // TOUCH_HANDLER_H
