#include "ui_modes.h"
#include "ui_layout.h"
#include "state/global_state.h"
#include "display.h"
#include "config/config.h"
#include "sensors/sensors.h"
#include <Wire.h>
#include <RTClib.h>

// External variables from main.cpp
extern Config cfg;
extern DisplayMode currentMode;
extern RTC_DS3231 rtc;

// Function prototype from main.cpp
const char* getMonthName(int month);

// Helper function to convert temperature based on user preference
inline float convertTemp(float celsius) {
  return cfg.use_fahrenheit ? ((celsius * 9.0 / 5.0) + 32.0) : celsius;
}

// ========== MONITOR MODE ==========

void drawMonitorMode() {
  gfx.fillScreen(COLOR_BG);

  // Header
  gfx.fillRect(0, 0, SCREEN_WIDTH, CommonLayout::HEADER_HEIGHT, COLOR_HEADER);
  gfx.setTextColor(COLOR_TEXT);
  gfx.setTextSize(MonitorLayout::HEADER_FONT_SIZE);
  gfx.setCursor(MonitorLayout::HEADER_TITLE_X, MonitorLayout::HEADER_TITLE_Y);
  gfx.print("FluidDash");

  // DateTime in header (right side)
  char buffer[40];
  if (network.rtcAvailable) {
    DateTime now = rtc.now();
    sprintf(buffer, "%s %02d  %02d:%02d:%02d",
            getMonthName(now.month()), now.day(), now.hour(), now.minute(), now.second());
  } else {
    sprintf(buffer, "No RTC");
  }
  gfx.setCursor(MonitorLayout::DATETIME_X, MonitorLayout::DATETIME_Y);
  gfx.print(buffer);

  // Dividers
  gfx.drawFastHLine(0, MonitorLayout::TOP_DIVIDER_Y, SCREEN_WIDTH, COLOR_LINE);
  gfx.drawFastHLine(0, MonitorLayout::MIDDLE_DIVIDER_Y, SCREEN_WIDTH, COLOR_LINE);
  gfx.drawFastVLine(MonitorLayout::VERTICAL_DIVIDER_X, MonitorLayout::TOP_DIVIDER_Y, MonitorLayout::VERTICAL_DIVIDER_HEIGHT, COLOR_LINE);

  // Left section - Driver temperatures
  gfx.setTextSize(MonitorLayout::TEMP_LABEL_FONT_SIZE);
  gfx.setTextColor(COLOR_TEXT);
  gfx.setCursor(MonitorLayout::TEMP_SECTION_X, MonitorLayout::TEMP_LABEL_Y);
  gfx.print("TEMPS:");

  // Default labels (used if no sensor mappings configured)
  const char* defaultLabels[] = {"X:", "YL:", "YR:", "Z:"};

  // Display driver temps by position (0=X, 1=YL, 2=YR, 3=Z)
  for (int pos = 0; pos < 4; pos++) {
    int rowY = MonitorLayout::TEMP_START_Y + pos * MonitorLayout::TEMP_ROW_SPACING;
    gfx.setCursor(MonitorLayout::TEMP_LABEL_X, rowY);
    gfx.setTextColor(COLOR_TEXT);

    // Find sensor assigned to this display position
    const SensorMapping* sensor = getSensorMappingByPosition(pos);
    float currentTemp = 0.0;
    float peakTemp = 0.0;

    if (sensor) {
      // Use sensor assigned to this position
      currentTemp = getTempByUID(sensor->uid);
      if (isnan(currentTemp)) currentTemp = 0.0;

      // Get peak temp for this position
      peakTemp = sensors.peakTemps[pos];

      // Show friendly name (truncated to 12 chars)
      char truncatedName[13];
      strlcpy(truncatedName, sensor->friendlyName, sizeof(truncatedName));
      gfx.print(truncatedName);
      gfx.print(":");
    } else {
      // No sensor assigned - show default label and use fallback temps array
      gfx.print(defaultLabels[pos]);
      currentTemp = sensors.temperatures[pos];
      peakTemp = sensors.peakTemps[pos];
    }

    // Current temp
    gfx.setTextSize(MonitorLayout::TEMP_VALUE_FONT_SIZE);
    gfx.setTextColor(currentTemp > cfg.temp_threshold_high ? COLOR_WARN : COLOR_VALUE);
    gfx.setCursor(MonitorLayout::TEMP_VALUE_X, rowY + MonitorLayout::TEMP_VALUE_Y_OFFSET);
    sprintf(buffer, "%d%s", (int)currentTemp, cfg.use_fahrenheit ? "F" : "C");
    gfx.print(buffer);

    // Peak temp to the right
    if (peakTemp > 0.0) {
      gfx.setTextSize(MonitorLayout::PEAK_TEMP_FONT_SIZE);
      gfx.setTextColor(COLOR_LINE);
      gfx.setCursor(MonitorLayout::PEAK_TEMP_X, rowY + MonitorLayout::PEAK_TEMP_Y_OFFSET);
      sprintf(buffer, "pk:%d%s", (int)peakTemp, cfg.use_fahrenheit ? "F" : "C");
      gfx.print(buffer);
    }

    gfx.setTextSize(1);
  }

  // Status section
  gfx.setTextColor(COLOR_TEXT);
  gfx.setCursor(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_LABEL_Y);
  gfx.print("STATUS:");

  gfx.setCursor(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_FAN_Y);
  gfx.setTextColor(COLOR_LINE);
  sprintf(buffer, "Fan: %d%% (%dRPM)", sensors.fanSpeed, sensors.fanRPM);
  gfx.print(buffer);

  gfx.setCursor(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_PSU_Y);
  sprintf(buffer, "PSU: %.1fV", sensors.psuVoltage);
  gfx.print(buffer);

  gfx.setCursor(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_FLUIDNC_Y);
  if (fluidnc.connected) {
    if (fluidnc.machineState == "RUN") gfx.setTextColor(COLOR_GOOD);
    else if (fluidnc.machineState == "ALARM") gfx.setTextColor(COLOR_WARN);
    else gfx.setTextColor(COLOR_VALUE);
    sprintf(buffer, "FluidNC: %s", fluidnc.machineState.c_str());
  } else {
    gfx.setTextColor(COLOR_WARN);
    sprintf(buffer, "FluidNC: Disconnected");
  }
  gfx.print(buffer);

  // Coordinates
  gfx.setTextColor(COLOR_TEXT);
  gfx.setCursor(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_COORDS_WCS_Y);
  if (cfg.coord_decimal_places == 3) {
    sprintf(buffer, "WCS: X:%.3f Y:%.3f Z:%.3f", fluidnc.wposX, fluidnc.wposY, fluidnc.wposZ);
  } else {
    sprintf(buffer, "WCS: X:%.2f Y:%.2f Z:%.2f", fluidnc.wposX, fluidnc.wposY, fluidnc.wposZ);
  }
  gfx.print(buffer);

  gfx.setCursor(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_COORDS_MCS_Y);
  if (cfg.coord_decimal_places == 3) {
    sprintf(buffer, "MCS: X:%.3f Y:%.3f Z:%.3f", fluidnc.posX, fluidnc.posY, fluidnc.posZ);
  } else {
    sprintf(buffer, "MCS: X:%.2f Y:%.2f Z:%.2f", fluidnc.posX, fluidnc.posY, fluidnc.posZ);
  }
  gfx.print(buffer);

  // Right section - Temperature graph
  gfx.setTextColor(COLOR_TEXT);
  gfx.setCursor(MonitorLayout::GRAPH_LABEL_X, MonitorLayout::GRAPH_LABEL_Y);
  gfx.print("TEMP HISTORY");

  if (cfg.show_temp_graph) {
    char graphLabel[40];
    if (cfg.graph_timespan_seconds >= 60) {
      sprintf(graphLabel, "(%d min)", cfg.graph_timespan_seconds / 60);
    } else {
      sprintf(graphLabel, "(%d sec)", cfg.graph_timespan_seconds);
    }
    gfx.setCursor(MonitorLayout::GRAPH_LABEL_X, MonitorLayout::GRAPH_TIMESPAN_Y);
    gfx.setTextColor(COLOR_LINE);
    gfx.print(graphLabel);

    // Draw the temperature history graph
    drawTempGraph(MonitorLayout::GRAPH_X, MonitorLayout::GRAPH_Y, MonitorLayout::GRAPH_WIDTH, MonitorLayout::GRAPH_HEIGHT);
  }
}

void updateMonitorMode() {
  // LovyanGFX version - only update dynamic parts (clear area, then redraw)
  char buffer[80];

  // Update DateTime in header
  if (network.rtcAvailable) {
    DateTime now = rtc.now();
    sprintf(buffer, "%s %02d  %02d:%02d:%02d",
            getMonthName(now.month()), now.day(), now.hour(), now.minute(), now.second());
  } else {
    sprintf(buffer, "No RTC");
  }
  gfx.fillRect(MonitorLayout::DATETIME_X, 0, MonitorLayout::DATETIME_WIDTH, CommonLayout::HEADER_HEIGHT, COLOR_HEADER);
  gfx.setTextSize(MonitorLayout::HEADER_FONT_SIZE);
  gfx.setTextColor(COLOR_TEXT);
  gfx.setCursor(MonitorLayout::DATETIME_X, MonitorLayout::DATETIME_Y);
  gfx.print(buffer);

  // Update temperature values and peaks
  for (int i = 0; i < 4; i++) {
    int rowY = MonitorLayout::TEMP_START_Y + i * MonitorLayout::TEMP_ROW_SPACING;

    // Clear the temperature display area for this driver
    gfx.fillRect(MonitorLayout::TEMP_VALUE_X, rowY + MonitorLayout::TEMP_VALUE_Y_OFFSET,
                 MonitorLayout::TEMP_VALUE_WIDTH, MonitorLayout::TEMP_VALUE_HEIGHT, COLOR_BG);

    // Current temp (convert to user's preferred unit)
    gfx.setTextSize(MonitorLayout::TEMP_VALUE_FONT_SIZE);
    gfx.setTextColor(sensors.temperatures[i] > cfg.temp_threshold_high ? COLOR_WARN : COLOR_VALUE);
    gfx.setCursor(MonitorLayout::TEMP_VALUE_X, rowY + MonitorLayout::TEMP_VALUE_Y_OFFSET);
    sprintf(buffer, "%d%s", (int)convertTemp(sensors.temperatures[i]), cfg.use_fahrenheit ? "F" : "C");
    gfx.print(buffer);

    // Peak temp (convert to user's preferred unit)
    gfx.setTextSize(MonitorLayout::PEAK_TEMP_FONT_SIZE);
    gfx.setTextColor(COLOR_LINE);
    gfx.setCursor(MonitorLayout::PEAK_TEMP_X, rowY + MonitorLayout::PEAK_TEMP_Y_OFFSET);
    sprintf(buffer, "pk:%d%s", (int)convertTemp(sensors.peakTemps[i]), cfg.use_fahrenheit ? "F" : "C");
    gfx.print(buffer);
  }

  // Update status section
  gfx.setTextSize(MonitorLayout::STATUS_LABEL_FONT_SIZE);

  // Fan
  gfx.fillRect(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_FAN_Y,
               MonitorLayout::STATUS_VALUE_WIDTH, MonitorLayout::STATUS_VALUE_HEIGHT, COLOR_BG);
  gfx.setTextColor(COLOR_LINE);
  gfx.setCursor(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_FAN_Y);
  sprintf(buffer, "Fan: %d%% (%dRPM)", sensors.fanSpeed, sensors.fanRPM);
  gfx.print(buffer);

  // PSU
  gfx.fillRect(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_PSU_Y,
               MonitorLayout::STATUS_VALUE_WIDTH, MonitorLayout::STATUS_VALUE_HEIGHT, COLOR_BG);
  gfx.setCursor(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_PSU_Y);
  gfx.setTextColor(COLOR_LINE);
  sprintf(buffer, "PSU: %.1fV", sensors.psuVoltage);
  gfx.print(buffer);

  // FluidNC Status
  gfx.fillRect(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_FLUIDNC_Y,
               MonitorLayout::STATUS_VALUE_WIDTH, MonitorLayout::STATUS_VALUE_HEIGHT, COLOR_BG);
  gfx.setCursor(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_FLUIDNC_Y);
  if (fluidnc.connected) {
    if (fluidnc.machineState == "RUN") gfx.setTextColor(COLOR_GOOD);
    else if (fluidnc.machineState == "ALARM") gfx.setTextColor(COLOR_WARN);
    else gfx.setTextColor(COLOR_VALUE);
    sprintf(buffer, "FluidNC: %s", fluidnc.machineState.c_str());
  } else {
    gfx.setTextColor(COLOR_WARN);
    sprintf(buffer, "FluidNC: Disconnected");
  }
  gfx.print(buffer);

  // WCS Coordinates
  gfx.fillRect(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_COORDS_WCS_Y,
               MonitorLayout::STATUS_VALUE_WIDTH, MonitorLayout::STATUS_VALUE_HEIGHT, COLOR_BG);
  gfx.setTextColor(COLOR_TEXT);
  gfx.setCursor(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_COORDS_WCS_Y);
  if (cfg.coord_decimal_places == 3) {
    sprintf(buffer, "WCS: X:%.3f Y:%.3f Z:%.3f", fluidnc.wposX, fluidnc.wposY, fluidnc.wposZ);
  } else {
    sprintf(buffer, "WCS: X:%.2f Y:%.2f Z:%.2f", fluidnc.wposX, fluidnc.wposY, fluidnc.wposZ);
  }
  gfx.print(buffer);

  // MCS Coordinates
  gfx.fillRect(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_COORDS_MCS_Y,
               MonitorLayout::STATUS_VALUE_WIDTH, MonitorLayout::STATUS_VALUE_HEIGHT, COLOR_BG);
  gfx.setCursor(MonitorLayout::STATUS_LABEL_X, MonitorLayout::STATUS_COORDS_MCS_Y);
  if (cfg.coord_decimal_places == 3) {
    sprintf(buffer, "MCS: X:%.3f Y:%.3f Z:%.3f", fluidnc.posX, fluidnc.posY, fluidnc.posZ);
  } else {
    sprintf(buffer, "MCS: X:%.2f Y:%.2f Z:%.2f", fluidnc.posX, fluidnc.posY, fluidnc.posZ);
  }
  gfx.print(buffer);

  // Update temperature graph (if enabled)
  if (cfg.show_temp_graph) {
    drawTempGraph(MonitorLayout::GRAPH_X, MonitorLayout::GRAPH_Y, MonitorLayout::GRAPH_WIDTH, MonitorLayout::GRAPH_HEIGHT);
  }
}
