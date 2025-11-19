#include "ui_modes.h"
#include "display.h"
#include "config/config.h"
#include "sensors/sensors.h"
#include <Wire.h>
#include <RTClib.h>

// External variables from main.cpp
extern Config cfg;
extern DisplayMode currentMode;
extern float temperatures[4];
extern float peakTemps[4];
extern float psuVoltage;
extern uint8_t fanSpeed;
extern uint16_t fanRPM;
extern bool fluidncConnected;
extern String machineState;
extern float posX, posY, posZ, posA;
extern float wposX, wposY, wposZ, wposA;
extern bool rtcAvailable;
extern RTC_DS3231 rtc;
extern float *tempHistory;
extern uint16_t historySize;

// Function prototype from main.cpp
const char* getMonthName(int month);

// ========== MONITOR MODE ==========

void drawMonitorMode() {
  gfx.fillScreen(COLOR_BG);

  // Header
  gfx.fillRect(0, 0, SCREEN_WIDTH, 25, COLOR_HEADER);
  gfx.setTextColor(COLOR_TEXT);
  gfx.setTextSize(2);
  gfx.setCursor(10, 6);
  gfx.print("FluidDash");

  // DateTime in header (right side)
  char buffer[40];
  if (rtcAvailable) {
    DateTime now = rtc.now();
    sprintf(buffer, "%s %02d  %02d:%02d:%02d",
            getMonthName(now.month()), now.day(), now.hour(), now.minute(), now.second());
  } else {
    sprintf(buffer, "No RTC");
  }
  gfx.setCursor(270, 6);
  gfx.print(buffer);

  // Dividers
  gfx.drawFastHLine(0, 25, SCREEN_WIDTH, COLOR_LINE);
  gfx.drawFastHLine(0, 175, SCREEN_WIDTH, COLOR_LINE);
  gfx.drawFastVLine(240, 25, 150, COLOR_LINE);

  // Left section - Driver temperatures
  gfx.setTextSize(1);
  gfx.setTextColor(COLOR_TEXT);
  gfx.setCursor(10, 30);
  gfx.print("TEMPS:");

  // Default labels (used if no sensor mappings configured)
  const char* defaultLabels[] = {"X:", "YL:", "YR:", "Z:"};

  // Display driver temps by position (0=X, 1=YL, 2=YR, 3=Z)
  for (int pos = 0; pos < 4; pos++) {
    gfx.setCursor(10, 50 + pos * 30);
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
      peakTemp = peakTemps[pos];

      // Show friendly name (truncated to 12 chars)
      char truncatedName[13];
      strlcpy(truncatedName, sensor->friendlyName, sizeof(truncatedName));
      gfx.print(truncatedName);
      gfx.print(":");
    } else {
      // No sensor assigned - show default label and use fallback temps array
      gfx.print(defaultLabels[pos]);
      currentTemp = temperatures[pos];
      peakTemp = peakTemps[pos];
    }

    // Current temp
    gfx.setTextSize(2);
    gfx.setTextColor(currentTemp > cfg.temp_threshold_high ? COLOR_WARN : COLOR_VALUE);
    gfx.setCursor(50, 47 + pos * 30);
    sprintf(buffer, "%d%s", (int)currentTemp, cfg.use_fahrenheit ? "F" : "C");
    gfx.print(buffer);

    // Peak temp to the right
    if (peakTemp > 0.0) {
      gfx.setTextSize(1);
      gfx.setTextColor(COLOR_LINE);
      gfx.setCursor(140, 52 + pos * 30);
      sprintf(buffer, "pk:%d%s", (int)peakTemp, cfg.use_fahrenheit ? "F" : "C");
      gfx.print(buffer);
    }

    gfx.setTextSize(1);
  }

  // Status section
  gfx.setTextColor(COLOR_TEXT);
  gfx.setCursor(10, 185);
  gfx.print("STATUS:");

  gfx.setCursor(10, 200);
  gfx.setTextColor(COLOR_LINE);
  sprintf(buffer, "Fan: %d%% (%dRPM)", fanSpeed, fanRPM);
  gfx.print(buffer);

  gfx.setCursor(10, 215);
  sprintf(buffer, "PSU: %.1fV", psuVoltage);
  gfx.print(buffer);

  gfx.setCursor(10, 230);
  if (fluidncConnected) {
    if (machineState == "RUN") gfx.setTextColor(COLOR_GOOD);
    else if (machineState == "ALARM") gfx.setTextColor(COLOR_WARN);
    else gfx.setTextColor(COLOR_VALUE);
    sprintf(buffer, "FluidNC: %s", machineState.c_str());
  } else {
    gfx.setTextColor(COLOR_WARN);
    sprintf(buffer, "FluidNC: Disconnected");
  }
  gfx.print(buffer);

  // Coordinates
  gfx.setTextColor(COLOR_TEXT);
  gfx.setCursor(10, 250);
  if (cfg.coord_decimal_places == 3) {
    sprintf(buffer, "WCS: X:%.3f Y:%.3f Z:%.3f", wposX, wposY, wposZ);
  } else {
    sprintf(buffer, "WCS: X:%.2f Y:%.2f Z:%.2f", wposX, wposY, wposZ);
  }
  gfx.print(buffer);

  gfx.setCursor(10, 265);
  if (cfg.coord_decimal_places == 3) {
    sprintf(buffer, "MCS: X:%.3f Y:%.3f Z:%.3f", posX, posY, posZ);
  } else {
    sprintf(buffer, "MCS: X:%.2f Y:%.2f Z:%.2f", posX, posY, posZ);
  }
  gfx.print(buffer);

  // Right section - Temperature graph
  gfx.setTextColor(COLOR_TEXT);
  gfx.setCursor(250, 30);
  gfx.print("TEMP HISTORY");

  if (cfg.show_temp_graph) {
    char graphLabel[40];
    if (cfg.graph_timespan_seconds >= 60) {
      sprintf(graphLabel, "(%d min)", cfg.graph_timespan_seconds / 60);
    } else {
      sprintf(graphLabel, "(%d sec)", cfg.graph_timespan_seconds);
    }
    gfx.setCursor(250, 40);
    gfx.setTextColor(COLOR_LINE);
    gfx.print(graphLabel);

    // Draw the temperature history graph
    drawTempGraph(250, 55, 220, 110);
  }
}

void updateMonitorMode() {
  // LovyanGFX version - only update dynamic parts (clear area, then redraw)
  char buffer[80];

  // Update DateTime in header
  if (rtcAvailable) {
    DateTime now = rtc.now();
    sprintf(buffer, "%s %02d  %02d:%02d:%02d",
            getMonthName(now.month()), now.day(), now.hour(), now.minute(), now.second());
  } else {
    sprintf(buffer, "No RTC");
  }
  gfx.fillRect(270, 0, 210, 25, COLOR_HEADER);
  gfx.setTextSize(2);
  gfx.setTextColor(COLOR_TEXT);
  gfx.setCursor(270, 6);
  gfx.print(buffer);

  // Update temperature values and peaks
  for (int i = 0; i < 4; i++) {
    // Clear the temperature display area for this driver
    gfx.fillRect(50, 47 + i * 30, 180, 20, COLOR_BG);

    // Current temp
    gfx.setTextSize(2);
    gfx.setTextColor(temperatures[i] > cfg.temp_threshold_high ? COLOR_WARN : COLOR_VALUE);
    gfx.setCursor(50, 47 + i * 30);
    sprintf(buffer, "%d%s", (int)temperatures[i], cfg.use_fahrenheit ? "F" : "C");
    gfx.print(buffer);

    // Peak temp
    gfx.setTextSize(1);
    gfx.setTextColor(COLOR_LINE);
    gfx.setCursor(140, 52 + i * 30);
    sprintf(buffer, "pk:%d%s", (int)peakTemps[i], cfg.use_fahrenheit ? "F" : "C");
    gfx.print(buffer);
  }

  // Update status section
  gfx.setTextSize(1);

  // Fan
  gfx.fillRect(10, 200, 220, 10, COLOR_BG);
  gfx.setTextColor(COLOR_LINE);
  gfx.setCursor(10, 200);
  sprintf(buffer, "Fan: %d%% (%dRPM)", fanSpeed, fanRPM);
  gfx.print(buffer);

  // PSU
  gfx.fillRect(10, 215, 220, 10, COLOR_BG);
  gfx.setCursor(10, 215);
  gfx.setTextColor(COLOR_LINE);
  sprintf(buffer, "PSU: %.1fV", psuVoltage);
  gfx.print(buffer);

  // FluidNC Status
  gfx.fillRect(10, 230, 220, 10, COLOR_BG);
  gfx.setCursor(10, 230);
  if (fluidncConnected) {
    if (machineState == "RUN") gfx.setTextColor(COLOR_GOOD);
    else if (machineState == "ALARM") gfx.setTextColor(COLOR_WARN);
    else gfx.setTextColor(COLOR_VALUE);
    sprintf(buffer, "FluidNC: %s", machineState.c_str());
  } else {
    gfx.setTextColor(COLOR_WARN);
    sprintf(buffer, "FluidNC: Disconnected");
  }
  gfx.print(buffer);

  // WCS Coordinates
  gfx.fillRect(10, 250, 220, 10, COLOR_BG);
  gfx.setTextColor(COLOR_TEXT);
  gfx.setCursor(10, 250);
  if (cfg.coord_decimal_places == 3) {
    sprintf(buffer, "WCS: X:%.3f Y:%.3f Z:%.3f", wposX, wposY, wposZ);
  } else {
    sprintf(buffer, "WCS: X:%.2f Y:%.2f Z:%.2f", wposX, wposY, wposZ);
  }
  gfx.print(buffer);

  // MCS Coordinates
  gfx.fillRect(10, 265, 220, 10, COLOR_BG);
  gfx.setCursor(10, 265);
  if (cfg.coord_decimal_places == 3) {
    sprintf(buffer, "MCS: X:%.3f Y:%.3f Z:%.3f", posX, posY, posZ);
  } else {
    sprintf(buffer, "MCS: X:%.2f Y:%.2f Z:%.2f", posX, posY, posZ);
  }
  gfx.print(buffer);

  // Update temperature graph (if enabled)
  if (cfg.show_temp_graph) {
    drawTempGraph(250, 55, 220, 110);
  }
}
