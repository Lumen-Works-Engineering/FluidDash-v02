#include "ui_modes.h"
#include "ui_layout.h"
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
  gfx.fillRect(0, 0, SCREEN_WIDTH, CommonLayout::HEADER_HEIGHT, COLOR_HEADER);
  gfx.setTextColor(COLOR_TEXT);
  gfx.setTextSize(NetworkLayout::TITLE_FONT_SIZE);
  gfx.setCursor(NetworkLayout::TITLE_X, NetworkLayout::TITLE_Y);
  gfx.print("NETWORK STATUS");

  gfx.drawFastHLine(0, CommonLayout::HEADER_HEIGHT, SCREEN_WIDTH, COLOR_LINE);

  gfx.setTextSize(NetworkLayout::STATUS_TITLE_FONT_SIZE);
  gfx.setTextColor(COLOR_TEXT);

  if (network.inAPMode) {
    // AP Mode display
    gfx.setCursor(NetworkLayout::AP_TITLE_X, NetworkLayout::AP_TITLE_Y);
    gfx.setTextColor(COLOR_WARN);
    gfx.print("WiFi Config Mode Active");

    gfx.setTextSize(NetworkLayout::AP_STEP_FONT_SIZE);
    gfx.setTextColor(COLOR_TEXT);
    gfx.setCursor(NetworkLayout::AP_STEP1_X, NetworkLayout::AP_STEP1_Y);
    gfx.print("1. Connect to WiFi network:");

    gfx.setTextSize(NetworkLayout::AP_SSID_FONT_SIZE);
    gfx.setTextColor(COLOR_VALUE);
    gfx.setCursor(NetworkLayout::AP_SSID_X, NetworkLayout::AP_SSID_Y);
    gfx.print("FluidDash-Setup");

    gfx.setTextSize(NetworkLayout::AP_STEP_FONT_SIZE);
    gfx.setTextColor(COLOR_TEXT);
    gfx.setCursor(NetworkLayout::AP_STEP2_X, NetworkLayout::AP_STEP2_Y);
    gfx.print("2. Open browser and go to:");

    gfx.setTextSize(NetworkLayout::AP_URL_FONT_SIZE);
    gfx.setTextColor(COLOR_VALUE);
    gfx.setCursor(NetworkLayout::AP_URL_X, NetworkLayout::AP_URL_Y);
    gfx.print("http://192.168.4.1");

    gfx.setTextSize(NetworkLayout::AP_STEP_FONT_SIZE);
    gfx.setTextColor(COLOR_TEXT);
    gfx.setCursor(NetworkLayout::AP_STEP3_X, NetworkLayout::AP_STEP3_Y);
    gfx.print("3. Configure your WiFi settings");

    gfx.setCursor(NetworkLayout::AP_BACKGROUND_INFO_X, NetworkLayout::AP_BACKGROUND_INFO_Y);
    gfx.setTextColor(COLOR_LINE);
    gfx.print("Temperature monitoring continues in background");

    // Show how to exit AP mode
    gfx.setTextColor(COLOR_ORANGE);
    gfx.setCursor(NetworkLayout::AP_EXIT_INFO_X, NetworkLayout::AP_EXIT_INFO_Y);
    gfx.print("Press button briefly to return to monitoring");

  } else {
    // Normal network status display
    if (WiFi.status() == WL_CONNECTED) {
      gfx.setCursor(NetworkLayout::STATUS_TITLE_X, NetworkLayout::STATUS_TITLE_Y);
      gfx.setTextColor(COLOR_GOOD);
      gfx.print("WiFi Connected");

      gfx.setTextSize(NetworkLayout::STATUS_ROW_FONT_SIZE);
      gfx.setTextColor(COLOR_TEXT);

      gfx.setCursor(NetworkLayout::STATUS_LABEL_X, NetworkLayout::STATUS_SSID_Y);
      gfx.print("SSID:");
      gfx.setTextColor(COLOR_VALUE);
      gfx.setCursor(NetworkLayout::STATUS_VALUE_X, NetworkLayout::STATUS_SSID_Y);
      gfx.print(WiFi.SSID());

      gfx.setTextColor(COLOR_TEXT);
      gfx.setCursor(NetworkLayout::STATUS_LABEL_X, NetworkLayout::STATUS_IP_Y);
      gfx.print("IP Address:");
      gfx.setTextColor(COLOR_VALUE);
      gfx.setCursor(NetworkLayout::STATUS_VALUE_X, NetworkLayout::STATUS_IP_Y);
      gfx.print(WiFi.localIP());

      gfx.setTextColor(COLOR_TEXT);
      gfx.setCursor(NetworkLayout::STATUS_LABEL_X, NetworkLayout::STATUS_SIGNAL_Y);
      gfx.print("Signal:");
      gfx.setTextColor(COLOR_VALUE);
      gfx.setCursor(NetworkLayout::STATUS_VALUE_X, NetworkLayout::STATUS_SIGNAL_Y);
      int rssi = WiFi.RSSI();
      gfx.printf("%d dBm", rssi);

      gfx.setTextColor(COLOR_TEXT);
      gfx.setCursor(NetworkLayout::STATUS_LABEL_X, NetworkLayout::STATUS_MDNS_Y);
      gfx.print("mDNS:");
      gfx.setTextColor(COLOR_VALUE);
      gfx.setCursor(NetworkLayout::STATUS_VALUE_X, NetworkLayout::STATUS_MDNS_Y);
      gfx.printf("http://%s.local", cfg.device_name);

      if (fluidnc.connected) {
        gfx.setTextColor(COLOR_TEXT);
        gfx.setCursor(NetworkLayout::STATUS_LABEL_X, NetworkLayout::STATUS_FLUIDNC_Y);
        gfx.print("FluidNC:");
        gfx.setTextColor(COLOR_GOOD);
        gfx.setCursor(NetworkLayout::STATUS_VALUE_X, NetworkLayout::STATUS_FLUIDNC_Y);
        gfx.print("Connected");
      } else {
        gfx.setTextColor(COLOR_TEXT);
        gfx.setCursor(NetworkLayout::STATUS_LABEL_X, NetworkLayout::STATUS_FLUIDNC_Y);
        gfx.print("FluidNC:");
        gfx.setTextColor(COLOR_WARN);
        gfx.setCursor(NetworkLayout::STATUS_VALUE_X, NetworkLayout::STATUS_FLUIDNC_Y);
        gfx.print("Disconnected");
      }

    } else {
      gfx.setCursor(NetworkLayout::NOT_CONNECTED_TITLE_X, NetworkLayout::NOT_CONNECTED_TITLE_Y);
      gfx.setTextColor(COLOR_WARN);
      gfx.print("WiFi Not Connected");

      gfx.setTextSize(NetworkLayout::STATUS_ROW_FONT_SIZE);
      gfx.setTextColor(COLOR_TEXT);
      gfx.setCursor(NetworkLayout::NOT_CONNECTED_INFO1_X, NetworkLayout::NOT_CONNECTED_INFO1_Y);
      gfx.print("Temperature monitoring active (standalone mode)");

      gfx.setCursor(NetworkLayout::NOT_CONNECTED_INFO2_X, NetworkLayout::NOT_CONNECTED_INFO2_Y);
      gfx.setTextColor(COLOR_ORANGE);
      gfx.print("To configure WiFi:");
    }

    // Instructions for entering AP mode
    gfx.setTextSize(NetworkLayout::INSTRUCTIONS_FONT_SIZE);
    gfx.setTextColor(COLOR_LINE);
    gfx.setCursor(NetworkLayout::INSTRUCTIONS_LINE1_X, NetworkLayout::INSTRUCTIONS_LINE1_Y);
    gfx.print("Hold button for 10 seconds to enter WiFi");
    gfx.setCursor(NetworkLayout::INSTRUCTIONS_LINE2_X, NetworkLayout::INSTRUCTIONS_LINE2_Y);
    gfx.print("configuration mode");
  }
}

void updateNetworkMode() {
  // Update time in header if needed - but network info is mostly static
  // Could add dynamic signal strength updates here
}
