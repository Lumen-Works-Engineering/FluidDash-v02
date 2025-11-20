#include "ui_modes.h"
#include "state/global_state.h"
#include "display.h"
#include "config/config.h"
#include <Wire.h>
#include <RTClib.h>
#include <WiFi.h>

// External variables from main.cpp
extern Config cfg;
extern DisplayMode currentMode;

// Function prototypes from main.cpp
void setupWebServer();
void enterSetupMode();
const char* getMonthName(int month);

// ========== HELPER FUNCTIONS ==========

void drawTempGraph(int x, int y, int w, int h) {
  gfx.fillRect(x, y, w, h, COLOR_BG);
  gfx.drawRect(x, y, w, h, COLOR_LINE);

  float minTemp = 10.0;
  float maxTemp = 60.0;

  // Draw temperature line
  for (int i = 1; i < history.historySize; i++) {
    int idx1 = (history.historyIndex + i - 1) % history.historySize;
    int idx2 = (history.historyIndex + i) % history.historySize;

    float temp1 = history.tempHistory[idx1];
    float temp2 = history.tempHistory[idx2];

    int x1 = x + ((i - 1) * w / history.historySize);
    int y1 = y + h - ((temp1 - minTemp) / (maxTemp - minTemp) * h);
    int x2 = x + (i * w / history.historySize);
    int y2 = y + h - ((temp2 - minTemp) / (maxTemp - minTemp) * h);

    y1 = constrain(y1, y, y + h);
    y2 = constrain(y2, y, y + h);

    // Color based on temperature
    uint16_t color;
    if (temp2 > cfg.temp_threshold_high) color = COLOR_WARN;
    else if (temp2 > cfg.temp_threshold_low) color = COLOR_ORANGE;
    else color = COLOR_GOOD;

    gfx.drawLine(x1, y1, x2, y2, color);
  }

  // Scale markers
  gfx.setTextSize(1);
  gfx.setTextColor(COLOR_LINE);
  gfx.setCursor(x + 3, y + 2);
  gfx.print("60");
  gfx.setCursor(x + 3, y + h / 2 - 5);
  gfx.print("35");
  gfx.setCursor(x + 3, y + h - 10);
  gfx.print("10");
}

// ========== BUTTON HANDLING ==========

void handleButton() {
  bool currentState = (digitalRead(BTN_MODE) == LOW);

  if (currentState && !timing.buttonPressed) {
    timing.buttonPressed = true;
    timing.buttonPressStart = millis();
  }
  else if (!currentState && timing.buttonPressed) {
    unsigned long pressDuration = millis() - timing.buttonPressStart;
    timing.buttonPressed = false;

    if (pressDuration >= 5000) {
      enterSetupMode();
    } else if (pressDuration < 1000) {
      cycleDisplayMode();
    }
  }
  else if (timing.buttonPressed && (millis() - timing.buttonPressStart >= 2000)) {
    showHoldProgress();
  }
}

void cycleDisplayMode() {
  currentMode = (DisplayMode)((currentMode + 1) % 4);  // Now we have 4 modes
  drawScreen();

  // Flash mode name
  gfx.fillRect(180, 140, 120, 40, COLOR_HEADER);
  gfx.setTextColor(COLOR_TEXT);
  gfx.setTextSize(2);
  gfx.setCursor(190, 150);

  switch(currentMode) {
    case MODE_MONITOR: gfx.print("MONITOR"); break;
    case MODE_ALIGNMENT: gfx.print("ALIGNMENT"); break;
    case MODE_GRAPH: gfx.print("GRAPH"); break;
    case MODE_NETWORK: gfx.print("NETWORK"); break;
  }

  delay(800);
  drawScreen();
}

void showHoldProgress() {
  unsigned long elapsed = millis() - timing.buttonPressStart;
  int progress = map(elapsed, 2000, 5000, 0, 100);
  progress = constrain(progress, 0, 100);

  gfx.fillRect(140, 280, 200, 30, COLOR_BG);
  gfx.drawRect(140, 280, 200, 30, COLOR_TEXT);
  gfx.setTextColor(COLOR_WARN);
  gfx.setTextSize(1);
  gfx.setCursor(145, 285);
  gfx.print("Hold for Setup...");

  int barWidth = map(progress, 0, 100, 0, 190);
  gfx.fillRect(145, 295, barWidth, 10, COLOR_WARN);

  gfx.setCursor(145, 307);
  gfx.print(10 - (elapsed / 1000));
  gfx.print(" sec");
}

void enterSetupMode() {
  Serial.println("Entering WiFi configuration AP mode...");

  // Stop any existing WiFi connection
  WiFi.disconnect();
  delay(100);

  // Start in AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP("FluidDash-Setup");
  network.inAPMode = true;

  Serial.print("AP started. IP: ");
  Serial.println(WiFi.softAPIP());

  // Start web server if not already running
  if (!network.webServerStarted) {
    Serial.println("Starting web server for AP mode...");
    setupWebServer();
    network.webServerStarted = true;
  }

  // Show AP mode screen
  currentMode = MODE_NETWORK;
  drawScreen();

  Serial.println("WiFi configuration AP active. Connect to 'FluidDash-Setup' network");
  Serial.println("Then visit http://192.168.4.1/wifi to configure");
}

const char* getMonthName(int month) {
  const char* months[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  return months[month];
}
