#include "ui_modes.h"
#include "ui_layout.h"
#include "state/global_state.h"
#include "display.h"
#include "config/config.h"
#include "sensors/sensors.h"

// External variables from main.cpp
extern Config cfg;

// Helper function to convert temperature based on user preference
inline float convertTemp(float celsius) {
  return cfg.use_fahrenheit ? ((celsius * 9.0 / 5.0) + 32.0) : celsius;
}

// ========== ALIGNMENT MODE ==========

void drawAlignmentMode() {
  gfx.fillScreen(COLOR_BG);

  // Header
  gfx.fillRect(0, 0, SCREEN_WIDTH, CommonLayout::HEADER_HEIGHT, COLOR_HEADER);
  gfx.setTextColor(COLOR_TEXT);
  gfx.setTextSize(AlignmentLayout::TITLE_FONT_SIZE);
  gfx.setCursor(AlignmentLayout::TITLE_X, AlignmentLayout::TITLE_Y);
  gfx.print("ALIGNMENT MODE");

  gfx.drawFastHLine(0, CommonLayout::HEADER_HEIGHT, SCREEN_WIDTH, COLOR_LINE);

  // Title
  gfx.setTextSize(AlignmentLayout::SUBTITLE_FONT_SIZE);
  gfx.setTextColor(COLOR_HEADER);
  gfx.setCursor(AlignmentLayout::SUBTITLE_X, AlignmentLayout::SUBTITLE_Y);
  gfx.print("WORK POSITION");

  // Detect if 4-axis machine (if A-axis is non-zero or moving)
  bool has4Axes = (fluidnc.posA != 0 || fluidnc.wposA != 0);

  if (has4Axes) {
    // 4-AXIS DISPLAY - Slightly smaller to fit all axes
    gfx.setTextSize(AlignmentLayout::COORD_4AXIS_FONT_SIZE);
    gfx.setTextColor(COLOR_VALUE);

    char coordFormat[20];
    if (cfg.coord_decimal_places == 3) {
      strcpy(coordFormat, "X:%9.3f");
    } else {
      strcpy(coordFormat, "X:%8.2f");
    }

    gfx.setCursor(AlignmentLayout::COORD_4AXIS_START_X, AlignmentLayout::COORD_4AXIS_START_Y);
    gfx.printf(coordFormat, fluidnc.wposX);

    coordFormat[0] = 'Y';
    gfx.setCursor(AlignmentLayout::COORD_4AXIS_START_X, AlignmentLayout::COORD_4AXIS_START_Y + AlignmentLayout::COORD_4AXIS_SPACING);
    gfx.printf(coordFormat, fluidnc.wposY);

    coordFormat[0] = 'Z';
    gfx.setCursor(AlignmentLayout::COORD_4AXIS_START_X, AlignmentLayout::COORD_4AXIS_START_Y + 2 * AlignmentLayout::COORD_4AXIS_SPACING);
    gfx.printf(coordFormat, fluidnc.wposZ);

    coordFormat[0] = 'A';
    gfx.setCursor(AlignmentLayout::COORD_4AXIS_START_X, AlignmentLayout::COORD_4AXIS_START_Y + 3 * AlignmentLayout::COORD_4AXIS_SPACING);
    gfx.printf(coordFormat, fluidnc.wposA);

    // Small info footer for 4-axis
    gfx.setTextSize(AlignmentLayout::MACHINE_POS_FONT_SIZE);
    gfx.setTextColor(COLOR_LINE);
    gfx.setCursor(AlignmentLayout::MACHINE_POS_X, AlignmentLayout::MACHINE_POS_Y);
    gfx.printf("Machine: X:%.1f Y:%.1f Z:%.1f A:%.1f", fluidnc.posX, fluidnc.posY, fluidnc.posZ, fluidnc.posA);
  } else {
    // 3-AXIS DISPLAY - Original large size
    gfx.setTextSize(AlignmentLayout::COORD_3AXIS_FONT_SIZE);
    gfx.setTextColor(COLOR_VALUE);

    char coordFormat[20];
    if (cfg.coord_decimal_places == 3) {
      strcpy(coordFormat, "X:%9.3f");
    } else {
      strcpy(coordFormat, "X:%8.2f");
    }

    gfx.setCursor(AlignmentLayout::COORD_3AXIS_START_X, AlignmentLayout::COORD_3AXIS_START_Y);
    gfx.printf(coordFormat, fluidnc.wposX);

    coordFormat[0] = 'Y';
    gfx.setCursor(AlignmentLayout::COORD_3AXIS_START_X, AlignmentLayout::COORD_3AXIS_START_Y + AlignmentLayout::COORD_3AXIS_SPACING);
    gfx.printf(coordFormat, fluidnc.wposY);

    coordFormat[0] = 'Z';
    gfx.setCursor(AlignmentLayout::COORD_3AXIS_START_X, AlignmentLayout::COORD_3AXIS_START_Y + 2 * AlignmentLayout::COORD_3AXIS_SPACING);
    gfx.printf(coordFormat, fluidnc.wposZ);

    // Small info footer for 3-axis
    gfx.setTextSize(AlignmentLayout::MACHINE_POS_FONT_SIZE);
    gfx.setTextColor(COLOR_LINE);
    gfx.setCursor(AlignmentLayout::MACHINE_POS_X, 270);
    gfx.printf("Machine: X:%.1f Y:%.1f Z:%.1f", fluidnc.posX, fluidnc.posY, fluidnc.posZ);
  }

  // Status line (same for both)
  gfx.setCursor(AlignmentLayout::MACHINE_POS_X, 285);
  if (fluidnc.machineState == "RUN") gfx.setTextColor(COLOR_GOOD);
  else if (fluidnc.machineState == "ALARM") gfx.setTextColor(COLOR_WARN);
  else gfx.setTextColor(COLOR_VALUE);
  gfx.printf("Status: %s", fluidnc.machineState.c_str());

  float maxTemp = sensors.temperatures[0];
  for (int i = 1; i < 4; i++) {
    if (sensors.temperatures[i] > maxTemp) maxTemp = sensors.temperatures[i];
  }

  gfx.setTextColor(maxTemp > cfg.temp_threshold_high ? COLOR_WARN : COLOR_LINE);
  gfx.setCursor(AlignmentLayout::MACHINE_POS_X, 300);
  gfx.printf("Temps:%.0f%s  Fan:%d%%  PSU:%.1fV",
             convertTemp(maxTemp),
             cfg.use_fahrenheit ? "F" : "C",
             sensors.fanSpeed,
             sensors.psuVoltage);
}

void updateAlignmentMode() {
  // Detect if 4-axis machine
  bool has4Axes = (fluidnc.posA != 0 || fluidnc.wposA != 0);

  if (has4Axes) {
    // 4-AXIS UPDATE
    gfx.setTextSize(AlignmentLayout::COORD_4AXIS_FONT_SIZE);
    gfx.setTextColor(COLOR_VALUE);

    char coordFormat[20];
    if (cfg.coord_decimal_places == 3) {
      strcpy(coordFormat, "%9.3f");
    } else {
      strcpy(coordFormat, "%8.2f");
    }

    // Update X
    gfx.fillRect(140, AlignmentLayout::COORD_4AXIS_START_Y, 330, 32, COLOR_BG);
    gfx.setCursor(140, AlignmentLayout::COORD_4AXIS_START_Y);
    gfx.printf(coordFormat, fluidnc.wposX);

    // Update Y
    gfx.fillRect(140, AlignmentLayout::COORD_4AXIS_START_Y + AlignmentLayout::COORD_4AXIS_SPACING, 330, 32, COLOR_BG);
    gfx.setCursor(140, AlignmentLayout::COORD_4AXIS_START_Y + AlignmentLayout::COORD_4AXIS_SPACING);
    gfx.printf(coordFormat, fluidnc.wposY);

    // Update Z
    gfx.fillRect(140, AlignmentLayout::COORD_4AXIS_START_Y + 2 * AlignmentLayout::COORD_4AXIS_SPACING, 330, 32, COLOR_BG);
    gfx.setCursor(140, AlignmentLayout::COORD_4AXIS_START_Y + 2 * AlignmentLayout::COORD_4AXIS_SPACING);
    gfx.printf(coordFormat, fluidnc.wposZ);

    // Update A
    gfx.fillRect(140, AlignmentLayout::COORD_4AXIS_START_Y + 3 * AlignmentLayout::COORD_4AXIS_SPACING, 330, 32, COLOR_BG);
    gfx.setCursor(140, AlignmentLayout::COORD_4AXIS_START_Y + 3 * AlignmentLayout::COORD_4AXIS_SPACING);
    gfx.printf(coordFormat, fluidnc.wposA);

    // Update footer
    gfx.setTextSize(AlignmentLayout::MACHINE_POS_FONT_SIZE);
    gfx.fillRect(90, AlignmentLayout::MACHINE_POS_Y, 390, 40, COLOR_BG);

    gfx.setTextColor(COLOR_LINE);
    gfx.setCursor(90, AlignmentLayout::MACHINE_POS_Y);
    gfx.printf("X:%.1f Y:%.1f Z:%.1f A:%.1f", fluidnc.posX, fluidnc.posY, fluidnc.posZ, fluidnc.posA);
  } else {
    // 3-AXIS UPDATE - Original code
    gfx.setTextSize(AlignmentLayout::COORD_3AXIS_FONT_SIZE);
    gfx.setTextColor(COLOR_VALUE);

    char coordFormat[20];
    if (cfg.coord_decimal_places == 3) {
      strcpy(coordFormat, "%9.3f");
    } else {
      strcpy(coordFormat, "%8.2f");
    }

    gfx.fillRect(150, AlignmentLayout::COORD_3AXIS_START_Y, 320, 38, COLOR_BG);
    gfx.setCursor(150, AlignmentLayout::COORD_3AXIS_START_Y);
    gfx.printf(coordFormat, fluidnc.wposX);

    gfx.fillRect(150, AlignmentLayout::COORD_3AXIS_START_Y + AlignmentLayout::COORD_3AXIS_SPACING, 320, 38, COLOR_BG);
    gfx.setCursor(150, AlignmentLayout::COORD_3AXIS_START_Y + AlignmentLayout::COORD_3AXIS_SPACING);
    gfx.printf(coordFormat, fluidnc.wposY);

    gfx.fillRect(150, AlignmentLayout::COORD_3AXIS_START_Y + 2 * AlignmentLayout::COORD_3AXIS_SPACING, 320, 38, COLOR_BG);
    gfx.setCursor(150, AlignmentLayout::COORD_3AXIS_START_Y + 2 * AlignmentLayout::COORD_3AXIS_SPACING);
    gfx.printf(coordFormat, fluidnc.wposZ);

    // Update footer
    gfx.setTextSize(AlignmentLayout::MACHINE_POS_FONT_SIZE);
    gfx.fillRect(90, 270, 390, 35, COLOR_BG);

    gfx.setTextColor(COLOR_LINE);
    gfx.setCursor(90, 270);
    gfx.printf("X:%.1f Y:%.1f Z:%.1f", fluidnc.posX, fluidnc.posY, fluidnc.posZ);
  }

  // Update status (same for both)
  gfx.setCursor(80, 285);
  if (fluidnc.machineState == "RUN") gfx.setTextColor(COLOR_GOOD);
  else if (fluidnc.machineState == "ALARM") gfx.setTextColor(COLOR_WARN);
  else gfx.setTextColor(COLOR_VALUE);
  gfx.printf("%s", fluidnc.machineState.c_str());

  float maxTemp = sensors.temperatures[0];
  for (int i = 1; i < 4; i++) {
    if (sensors.temperatures[i] > maxTemp) maxTemp = sensors.temperatures[i];
  }

  gfx.setTextColor(maxTemp > cfg.temp_threshold_high ? COLOR_WARN : COLOR_LINE);
  gfx.setCursor(90, 300);
  gfx.printf("%.0f%s  Fan:%d%%  PSU:%.1fV",
             convertTemp(maxTemp),
             cfg.use_fahrenheit ? "F" : "C",
             sensors.fanSpeed,
             sensors.psuVoltage);
}
