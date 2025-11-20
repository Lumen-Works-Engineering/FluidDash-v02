#include "utils.h"
#include "config/config.h"
#include "state/global_state.h"

// ========== Memory Management ==========

void allocateHistoryBuffer() {
  // Calculate required buffer size with safety limit
  history.historySize = cfg.graph_timespan_seconds / cfg.graph_update_interval;

  // Limit max buffer size to prevent excessive memory usage (max 2000 points = 8KB)
  const uint16_t MAX_BUFFER_SIZE = 2000;
  if (history.historySize > MAX_BUFFER_SIZE) {
    Serial.printf("Warning: Buffer size %d exceeds limit, capping at %d\n", history.historySize, MAX_BUFFER_SIZE);
    history.historySize = MAX_BUFFER_SIZE;
  }

  // Reallocate if needed
  if (history.tempHistory != nullptr) {
    free(history.tempHistory);
    history.tempHistory = nullptr;
  }

  history.tempHistory = (float*)malloc(history.historySize * sizeof(float));

  // Check if allocation succeeded
  if (history.tempHistory == nullptr) {
    Serial.println("ERROR: Failed to allocate history buffer! Restarting...");
    delay(2000);
    ESP.restart();
  }

  // Initialize
  for (int i = 0; i < history.historySize; i++) {
    history.tempHistory[i] = 20.0;
  }

  history.historyIndex = 0;

  Serial.printf("History buffer: %d points (%d seconds, %d bytes)\n",
                history.historySize, cfg.graph_timespan_seconds, history.historySize * sizeof(float));
}

// ========== Watchdog Functions ==========
// Note: enableLoopWDT() and feedLoopWDT() are provided by the ESP32 Arduino framework
// in esp32-hal-misc.c, so we don't need to implement them here.
