#include "ui_modes.h"
#include "display.h"
#include "config/config.h"
#include "sensors/sensors.h"

// External variables from main.cpp
extern Config cfg;
extern float temperatures[4];
extern float psuVoltage;
extern uint8_t fanSpeed;
extern String machineState;
extern float posX, posY, posZ, posA;
extern float wposX, wposY, wposZ, wposA;

// ========== ALIGNMENT MODE ==========

void drawAlignmentMode() {
  gfx.fillScreen(COLOR_BG);

  // Header
  gfx.fillRect(0, 0, SCREEN_WIDTH, 25, COLOR_HEADER);
  gfx.setTextColor(COLOR_TEXT);
  gfx.setTextSize(2);
  gfx.setCursor(140, 6);
  gfx.print("ALIGNMENT MODE");

  gfx.drawFastHLine(0, 25, SCREEN_WIDTH, COLOR_LINE);

  // Title
  gfx.setTextSize(2);
  gfx.setTextColor(COLOR_HEADER);
  gfx.setCursor(150, 40);
  gfx.print("WORK POSITION");

  // Detect if 4-axis machine (if A-axis is non-zero or moving)
  bool has4Axes = (posA != 0 || wposA != 0);

  if (has4Axes) {
    // 4-AXIS DISPLAY - Slightly smaller to fit all axes
    gfx.setTextSize(4);
    gfx.setTextColor(COLOR_VALUE);

    char coordFormat[20];
    if (cfg.coord_decimal_places == 3) {
      strcpy(coordFormat, "X:%9.3f");
    } else {
      strcpy(coordFormat, "X:%8.2f");
    }

    gfx.setCursor(40, 75);
    gfx.printf(coordFormat, wposX);

    coordFormat[0] = 'Y';
    gfx.setCursor(40, 120);
    gfx.printf(coordFormat, wposY);

    coordFormat[0] = 'Z';
    gfx.setCursor(40, 165);
    gfx.printf(coordFormat, wposZ);

    coordFormat[0] = 'A';
    gfx.setCursor(40, 210);
    gfx.printf(coordFormat, wposA);

    // Small info footer for 4-axis
    gfx.setTextSize(1);
    gfx.setTextColor(COLOR_LINE);
    gfx.setCursor(10, 265);
    gfx.printf("Machine: X:%.1f Y:%.1f Z:%.1f A:%.1f", posX, posY, posZ, posA);
  } else {
    // 3-AXIS DISPLAY - Original large size
    gfx.setTextSize(5);
    gfx.setTextColor(COLOR_VALUE);

    char coordFormat[20];
    if (cfg.coord_decimal_places == 3) {
      strcpy(coordFormat, "X:%9.3f");
    } else {
      strcpy(coordFormat, "X:%8.2f");
    }

    gfx.setCursor(40, 90);
    gfx.printf(coordFormat, wposX);

    coordFormat[0] = 'Y';
    gfx.setCursor(40, 145);
    gfx.printf(coordFormat, wposY);

    coordFormat[0] = 'Z';
    gfx.setCursor(40, 200);
    gfx.printf(coordFormat, wposZ);

    // Small info footer for 3-axis
    gfx.setTextSize(1);
    gfx.setTextColor(COLOR_LINE);
    gfx.setCursor(10, 270);
    gfx.printf("Machine: X:%.1f Y:%.1f Z:%.1f", posX, posY, posZ);
  }

  // Status line (same for both)
  gfx.setCursor(10, 285);
  if (machineState == "RUN") gfx.setTextColor(COLOR_GOOD);
  else if (machineState == "ALARM") gfx.setTextColor(COLOR_WARN);
  else gfx.setTextColor(COLOR_VALUE);
  gfx.printf("Status: %s", machineState.c_str());

  float maxTemp = temperatures[0];
  for (int i = 1; i < 4; i++) {
    if (temperatures[i] > maxTemp) maxTemp = temperatures[i];
  }

  gfx.setTextColor(maxTemp > cfg.temp_threshold_high ? COLOR_WARN : COLOR_LINE);
  gfx.setCursor(10, 300);
  gfx.printf("Temps:%.0fC  Fan:%d%%  PSU:%.1fV", maxTemp, fanSpeed, psuVoltage);
}

void updateAlignmentMode() {
  // Detect if 4-axis machine
  bool has4Axes = (posA != 0 || wposA != 0);

  if (has4Axes) {
    // 4-AXIS UPDATE
    gfx.setTextSize(4);
    gfx.setTextColor(COLOR_VALUE);

    char coordFormat[20];
    if (cfg.coord_decimal_places == 3) {
      strcpy(coordFormat, "%9.3f");
    } else {
      strcpy(coordFormat, "%8.2f");
    }

    // Update X
    gfx.fillRect(140, 75, 330, 32, COLOR_BG);
    gfx.setCursor(140, 75);
    gfx.printf(coordFormat, wposX);

    // Update Y
    gfx.fillRect(140, 120, 330, 32, COLOR_BG);
    gfx.setCursor(140, 120);
    gfx.printf(coordFormat, wposY);

    // Update Z
    gfx.fillRect(140, 165, 330, 32, COLOR_BG);
    gfx.setCursor(140, 165);
    gfx.printf(coordFormat, wposZ);

    // Update A
    gfx.fillRect(140, 210, 330, 32, COLOR_BG);
    gfx.setCursor(140, 210);
    gfx.printf(coordFormat, wposA);

    // Update footer
    gfx.setTextSize(1);
    gfx.fillRect(90, 265, 390, 40, COLOR_BG);

    gfx.setTextColor(COLOR_LINE);
    gfx.setCursor(90, 265);
    gfx.printf("X:%.1f Y:%.1f Z:%.1f A:%.1f", posX, posY, posZ, posA);
  } else {
    // 3-AXIS UPDATE - Original code
    gfx.setTextSize(5);
    gfx.setTextColor(COLOR_VALUE);

    char coordFormat[20];
    if (cfg.coord_decimal_places == 3) {
      strcpy(coordFormat, "%9.3f");
    } else {
      strcpy(coordFormat, "%8.2f");
    }

    gfx.fillRect(150, 90, 320, 38, COLOR_BG);
    gfx.setCursor(150, 90);
    gfx.printf(coordFormat, wposX);

    gfx.fillRect(150, 145, 320, 38, COLOR_BG);
    gfx.setCursor(150, 145);
    gfx.printf(coordFormat, wposY);

    gfx.fillRect(150, 200, 320, 38, COLOR_BG);
    gfx.setCursor(150, 200);
    gfx.printf(coordFormat, wposZ);

    // Update footer
    gfx.setTextSize(1);
    gfx.fillRect(90, 270, 390, 35, COLOR_BG);

    gfx.setTextColor(COLOR_LINE);
    gfx.setCursor(90, 270);
    gfx.printf("X:%.1f Y:%.1f Z:%.1f", posX, posY, posZ);
  }

  // Update status (same for both)
  gfx.setCursor(80, 285);
  if (machineState == "RUN") gfx.setTextColor(COLOR_GOOD);
  else if (machineState == "ALARM") gfx.setTextColor(COLOR_WARN);
  else gfx.setTextColor(COLOR_VALUE);
  gfx.printf("%s", machineState.c_str());

  float maxTemp = temperatures[0];
  for (int i = 1; i < 4; i++) {
    if (temperatures[i] > maxTemp) maxTemp = temperatures[i];
  }

  gfx.setTextColor(maxTemp > cfg.temp_threshold_high ? COLOR_WARN : COLOR_LINE);
  gfx.setCursor(90, 300);
  gfx.printf("%.0fC  Fan:%d%%  PSU:%.1fV", maxTemp, fanSpeed, psuVoltage);
}
