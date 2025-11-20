#ifndef DISPLAY_H
#define DISPLAY_H

#include <LovyanGFX.hpp>
#include "config/pins.h"

// LovyanGFX Display Configuration for ST7796 480x320
class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7796 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;
    lgfx::Touch_XPT2046 _touch_instance;  // XPT2046 touchscreen controller
public:
    LGFX(void);
};

// Global display instance
extern LGFX gfx;

// Display control functions
void showSplashScreen();
void setBrightness(uint8_t brightness);

#endif // DISPLAY_H
