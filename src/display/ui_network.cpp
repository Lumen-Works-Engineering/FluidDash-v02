#include "ui_modes.h"
#include "display.h"
#include "config/config.h"
#include "state/global_state.h"
#include <WiFi.h>

// External variables from main.cpp
extern Config cfg;
extern bool webServerStarted;
// ========== NETWORK MODE ==========

void drawNetworkMode() {
  gfx.fillScreen(COLOR_BG);

  // Header
  gfx.fillRect(0, 0, SCREEN_WIDTH, 25, COLOR_HEADER);
  gfx.setTextColor(COLOR_TEXT);
  gfx.setTextSize(2);
  gfx.setCursor(120, 6);
  gfx.print("NETWORK STATUS");

  gfx.drawFastHLine(0, 25, SCREEN_WIDTH, COLOR_LINE);

  gfx.setTextSize(2);
  gfx.setTextColor(COLOR_TEXT);

  if (network.inAPMode) {
    // AP Mode display
    gfx.setCursor(60, 50);
    gfx.setTextColor(COLOR_WARN);
    gfx.print("WiFi Config Mode Active");

    gfx.setTextSize(1);
    gfx.setTextColor(COLOR_TEXT);
    gfx.setCursor(10, 90);
    gfx.print("1. Connect to WiFi network:");

    gfx.setTextSize(2);
    gfx.setTextColor(COLOR_VALUE);
    gfx.setCursor(40, 110);
    gfx.print("FluidDash-Setup");

    gfx.setTextSize(1);
    gfx.setTextColor(COLOR_TEXT);
    gfx.setCursor(10, 145);
    gfx.print("2. Open browser and go to:");

    gfx.setTextSize(2);
    gfx.setTextColor(COLOR_VALUE);
    gfx.setCursor(80, 165);
    gfx.print("http://192.168.4.1");

    gfx.setTextSize(1);
    gfx.setTextColor(COLOR_TEXT);
    gfx.setCursor(10, 200);
    gfx.print("3. Configure your WiFi settings");

    gfx.setCursor(10, 230);
    gfx.setTextColor(COLOR_LINE);
    gfx.print("Temperature monitoring continues in background");

    // Show to exit AP mode
    gfx.setTextColor(COLOR_ORANGE);
    gfx.setCursor(10, 270);
    gfx.print("Press button briefly to return to monitoring");

  } else {
    // Normal network status display
    if (WiFi.status() == WL_CONNECTED) {
      gfx.setCursor(130, 50);
      gfx.setTextColor(COLOR_GOOD);
      gfx.print("WiFi Connected");

      gfx.setTextSize(1);
      gfx.setTextColor(COLOR_TEXT);

      gfx.setCursor(10, 90);
      gfx.print("SSID:");
      gfx.setTextColor(COLOR_VALUE);
      gfx.setCursor(80, 90);
      gfx.print(WiFi.SSID());

      gfx.setTextColor(COLOR_TEXT);
      gfx.setCursor(10, 115);
      gfx.print("IP Address:");
      gfx.setTextColor(COLOR_VALUE);
      gfx.setCursor(80, 115);
      gfx.print(WiFi.localIP());

      gfx.setTextColor(COLOR_TEXT);
      gfx.setCursor(10, 140);
      gfx.print("Signal:");
      gfx.setTextColor(COLOR_VALUE);
      gfx.setCursor(80, 140);
      int rssi = WiFi.RSSI();
      gfx.printf("%d dBm", rssi);

      gfx.setTextColor(COLOR_TEXT);
      gfx.setCursor(10, 165);
      gfx.print("mDNS:");
      gfx.setTextColor(COLOR_VALUE);
      gfx.setCursor(80, 165);
      gfx.printf("http://%s.local", cfg.device_name);

      if (fluidnc.connected) {
        gfx.setTextColor(COLOR_TEXT);
        gfx.setCursor(10, 190);
        gfx.print("FluidNC:");
        gfx.setTextColor(COLOR_GOOD);
        gfx.setCursor(80, 190);
        gfx.print("Connected");
      } else {
        gfx.setTextColor(COLOR_TEXT);
        gfx.setCursor(10, 190);
        gfx.print("FluidNC:");
        gfx.setTextColor(COLOR_WARN);
        gfx.setCursor(80, 190);
        gfx.print("Disconnected");
      }

    } else {
      gfx.setCursor(120, 50);
      gfx.setTextColor(COLOR_WARN);
      gfx.print("WiFi Not Connected");

      gfx.setTextSize(1);
      gfx.setTextColor(COLOR_TEXT);
      gfx.setCursor(10, 100);
      gfx.print("Temperature monitoring active (standalone mode)");

      gfx.setCursor(10, 130);
      gfx.setTextColor(COLOR_ORANGE);
      gfx.print("To configure WiFi:");
    }

    // Instructions for entering AP mode
    gfx.setTextSize(1);
    gfx.setTextColor(COLOR_LINE);
    gfx.setCursor(10, 250);
    gfx.print("Hold button for 10 seconds to enter WiFi");
    gfx.setCursor(10, 265);
    gfx.print("configuration mode");
  }
}

void updateNetworkMode() {
  // Update time in header if needed - but network info is mostly static
  // Could add dynamic signal strength updates here
}
