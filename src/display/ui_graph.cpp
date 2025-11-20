#include "ui_modes.h"
#include "ui_layout.h"
#include "state/global_state.h"
#include "display.h"
#include "config/config.h"

// External variables from main.cpp
extern Config cfg;
extern int cfg_graph_timespan_seconds;

// ========== GRAPH MODE ==========

void drawGraphMode() {
  gfx.fillScreen(COLOR_BG);

  // Header
  gfx.fillRect(0, 0, SCREEN_WIDTH, CommonLayout::HEADER_HEIGHT, COLOR_HEADER);
  gfx.setTextColor(COLOR_TEXT);
  gfx.setTextSize(GraphLayout::TITLE_FONT_SIZE);
  gfx.setCursor(GraphLayout::TITLE_X, GraphLayout::TITLE_Y);
  gfx.print("TEMPERATURE HISTORY");

  char timeLabel[40];
  if (cfg.graph_timespan_seconds >= 60) {
    sprintf(timeLabel, " - %d minutes", cfg.graph_timespan_seconds / 60);
  } else {
    sprintf(timeLabel, " - %d seconds", cfg.graph_timespan_seconds);
  }
  gfx.setTextSize(GraphLayout::TIMESPAN_LABEL_FONT_SIZE);
  gfx.setCursor(GraphLayout::TIMESPAN_LABEL_X, GraphLayout::TIMESPAN_LABEL_Y);
  gfx.print(timeLabel);

  gfx.drawFastHLine(0, CommonLayout::HEADER_HEIGHT, SCREEN_WIDTH, COLOR_LINE);

  // Full screen graph
  drawTempGraph(GraphLayout::GRAPH_X, GraphLayout::GRAPH_Y, GraphLayout::GRAPH_WIDTH, GraphLayout::GRAPH_HEIGHT);
}

void updateGraphMode() {
  // Redraw graph
  drawTempGraph(GraphLayout::GRAPH_X, GraphLayout::GRAPH_Y, GraphLayout::GRAPH_WIDTH, GraphLayout::GRAPH_HEIGHT);
}
