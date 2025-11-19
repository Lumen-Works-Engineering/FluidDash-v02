#include "ui_modes.h"
#include "display.h"
#include "config/config.h"

// External variables from main.cpp
extern Config cfg;
extern int cfg_graph_timespan_seconds;

// ========== GRAPH MODE ==========

void drawGraphMode() {
  gfx.fillScreen(COLOR_BG);

  // Header
  gfx.fillRect(0, 0, SCREEN_WIDTH, 25, COLOR_HEADER);
  gfx.setTextColor(COLOR_TEXT);
  gfx.setTextSize(2);
  gfx.setCursor(100, 6);
  gfx.print("TEMPERATURE HISTORY");

  char timeLabel[40];
  if (cfg.graph_timespan_seconds >= 60) {
    sprintf(timeLabel, " - %d minutes", cfg.graph_timespan_seconds / 60);
  } else {
    sprintf(timeLabel, " - %d seconds", cfg.graph_timespan_seconds);
  }
  gfx.setTextSize(1);
  gfx.setCursor(330, 10);
  gfx.print(timeLabel);

  gfx.drawFastHLine(0, 25, SCREEN_WIDTH, COLOR_LINE);

  // Full screen graph
  drawTempGraph(20, 40, 440, 270);
}

void updateGraphMode() {
  // Redraw graph
  drawTempGraph(20, 40, 440, 270);
}

