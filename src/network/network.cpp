#include "network.h"
#include "../state/global_state.h"
#include "config/config.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebSocketsClient.h>
#include <ESPmDNS.h>

// ========== WiFiManager Setup ==========

void setupWiFiManager() {
  // WiFi Manager custom parameters
  WiFiManagerParameter custom_fluidnc_ip("fluidnc_ip", "FluidNC IP Address", cfg.fluidnc_ip, 16);
  WiFiManagerParameter custom_device_name("dev_name", "Device Name", cfg.device_name, 32);

  // Add parameters to WiFiManager
  wm.addParameter(&custom_fluidnc_ip);
  wm.addParameter(&custom_device_name);
}

// ========== FluidNC Connection ==========

void connectFluidNC() {
    Serial.printf("[FluidNC] Attempting to connect to ws://%s:%d/ws\n",
                  cfg.fluidnc_ip, cfg.fluidnc_port);
    webSocket.begin(cfg.fluidnc_ip, cfg.fluidnc_port, "/ws");  // Add /ws path
    webSocket.onEvent(fluidNCWebSocketEvent);
    webSocket.setReconnectInterval(5000);
    Serial.println("[FluidNC] WebSocket initialized, waiting for connection...");
}

void discoverFluidNC() {
  Serial.println("Auto-discovering FluidNC...");

  // Feed watchdog before potentially long mDNS query
  yield();
  delay(10);  // Brief delay to let system settle

  // Try mDNS discovery first
  Serial.println("[mDNS] Querying for FluidNC services...");
  int n = MDNS.queryService("http", "tcp");

  // Feed watchdog after mDNS query
  yield();

  Serial.printf("[mDNS] Found %d service(s)\n", n);

  for (int i = 0; i < n; i++) {
    yield();  // Feed watchdog in loop

    String hostname = MDNS.hostname(i);
    Serial.printf("[mDNS] Service %d: %s\n", i, hostname.c_str());

    if (hostname.indexOf("fluidnc") >= 0) {
      IPAddress ip = MDNS.IP(i);
      strlcpy(cfg.fluidnc_ip, ip.toString().c_str(), sizeof(cfg.fluidnc_ip));
      Serial.printf("Found FluidNC at: %s\n", cfg.fluidnc_ip);
      connectFluidNC();
      return;
    }
  }

  // Fallback to configured IP
  Serial.println("No FluidNC found via mDNS, using configured IP");
  connectFluidNC();
}

void fluidNCWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("[FluidNC] Disconnected!");
            fluidnc.connected = false;
            fluidnc.machineState = "OFFLINE";
            break;

        case WStype_CONNECTED:
            Serial.printf("[FluidNC] Connected to: %s\n", payload);
            fluidnc.connected = true;
            fluidnc.machineState = "IDLE";

            // DON'T send ReportInterval - FluidNC doesn't support it
            // We'll use manual polling with ? status requests
            fluidnc.reportingSetupTime = millis();
            break;

        case WStype_TEXT:
            {
                char* msg = (char*)payload;
                if (fluidnc.debugWebSocket) {
                    Serial.printf("[FluidNC] RX TEXT (%d bytes): ", length);
                    for(size_t i = 0; i < length; i++) {
                        Serial.print(msg[i]);
                    }
                    Serial.println();
                }

                String msgStr = String(msg);
                if (msgStr.startsWith("<")) {
                    parseFluidNCStatus(msgStr);
                } else if (msgStr.startsWith("ALARM:")) {
                    fluidnc.machineState = "ALARM";
                    parseFluidNCStatus(msgStr);
                }
            }
            break;

        case WStype_BIN:
            {
                // FluidNC sends status as BINARY data
                if (fluidnc.debugWebSocket) {
                    Serial.printf("[FluidNC] RX BINARY (%d bytes): ", length);
                }

                // Convert binary payload to null-terminated string
                char* msg = (char*)malloc(length + 1);
                if (msg != nullptr) {
                    memcpy(msg, payload, length);
                    msg[length] = '\0';

                    if (fluidnc.debugWebSocket) {
                        Serial.println(msg);
                    }

                    // Parse the status message
                    String msgStr = String(msg);
                    parseFluidNCStatus(msgStr);

                    free(msg);
                } else {
                    Serial.println("[FluidNC] ERROR: Failed to allocate memory");
                }
            }
            break;

        case WStype_ERROR:
            Serial.println("[FluidNC] WebSocket Error!");
            break;

        case WStype_PING:
            // Ping/pong for keep-alive - normal, no logging needed
            break;

        case WStype_PONG:
            break;

        default:
            if (fluidnc.debugWebSocket) {
                Serial.printf("[FluidNC] Event type: %d\n", type);
            }
            break;
    }
}

void parseFluidNCStatus(String status) {
    String oldState = fluidnc.machineState;

    // Parse state (between < and |)
    int stateEnd = status.indexOf('|');
    if (stateEnd > 0) {
        String state = status.substring(1, stateEnd);
        fluidnc.machineState = state;
        fluidnc.machineState.toUpperCase();

        // Job tracking
        if (oldState != "RUN" && fluidnc.machineState == "RUN") {
            fluidnc.jobStartTime = millis();
            fluidnc.isJobRunning = true;
        }
        if (oldState == "RUN" && fluidnc.machineState != "RUN") {
            fluidnc.isJobRunning = false;
        }
    }

    // Parse MPos (Machine Position) - supports 3 or 4 axes
    int mposIndex = status.indexOf("MPos:");
    if (mposIndex >= 0) {
        int endIndex = status.indexOf('|', mposIndex);
        if (endIndex < 0) endIndex = status.indexOf('>', mposIndex);
        String posStr = status.substring(mposIndex + 5, endIndex);

        // Count commas to determine axis count
        int commaCount = 0;
        for (int i = 0; i < posStr.length(); i++) {
            if (posStr.charAt(i) == ',') commaCount++;
        }

        if (commaCount >= 3) {
            // 4-axis machine
            sscanf(posStr.c_str(), "%f,%f,%f,%f", &fluidnc.posX, &fluidnc.posY, &fluidnc.posZ, &fluidnc.posA);
        } else {
            // 3-axis machine
            sscanf(posStr.c_str(), "%f,%f,%f", &fluidnc.posX, &fluidnc.posY, &fluidnc.posZ);
            fluidnc.posA = 0;
        }
    }

    // Parse WPos (Work Position) if present
    int wposIndex = status.indexOf("WPos:");
    if (wposIndex >= 0) {
        int endIndex = status.indexOf('|', wposIndex);
        if (endIndex < 0) endIndex = status.indexOf('>', wposIndex);
        String posStr = status.substring(wposIndex + 5, endIndex);

        int commaCount = 0;
        for (int i = 0; i < posStr.length(); i++) {
            if (posStr.charAt(i) == ',') commaCount++;
        }

        if (commaCount >= 3) {
            sscanf(posStr.c_str(), "%f,%f,%f,%f", &fluidnc.wposX, &fluidnc.wposY, &fluidnc.wposZ, &fluidnc.wposA);
        } else {
            sscanf(posStr.c_str(), "%f,%f,%f", &fluidnc.wposX, &fluidnc.wposY, &fluidnc.wposZ);
            fluidnc.wposA = 0;
        }
    } else {
        // No WPos - use MPos
        fluidnc.wposX = fluidnc.posX;
        fluidnc.wposY = fluidnc.posY;
        fluidnc.wposZ = fluidnc.posZ;
        fluidnc.wposA = fluidnc.posA;
    }

    // Parse WCO (Work Coordinate Offset) if present
    int wcoIndex = status.indexOf("WCO:");
    if (wcoIndex >= 0) {
        int endIndex = status.indexOf('|', wcoIndex);
        if (endIndex < 0) endIndex = status.indexOf('>', wcoIndex);
        String wcoStr = status.substring(wcoIndex + 4, endIndex);

        int commaCount = 0;
        for (int i = 0; i < wcoStr.length(); i++) {
            if (wcoStr.charAt(i) == ',') commaCount++;
        }

        if (commaCount >= 3) {
            sscanf(wcoStr.c_str(), "%f,%f,%f,%f", &fluidnc.wcoX, &fluidnc.wcoY, &fluidnc.wcoZ, &fluidnc.wcoA);
        } else {
            sscanf(wcoStr.c_str(), "%f,%f,%f", &fluidnc.wcoX, &fluidnc.wcoY, &fluidnc.wcoZ);
            fluidnc.wcoA = 0;
        }

        // Calculate WPos from MPos - WCO
        fluidnc.wposX = fluidnc.posX - fluidnc.wcoX;
        fluidnc.wposY = fluidnc.posY - fluidnc.wcoY;
        fluidnc.wposZ = fluidnc.posZ - fluidnc.wcoZ;
        fluidnc.wposA = fluidnc.posA - fluidnc.wcoA;
    }

    // Parse FS (Feed rate and Spindle speed)
    int fsIndex = status.indexOf("FS:");
    if (fsIndex >= 0) {
        int endIndex = status.indexOf('|', fsIndex);
        if (endIndex < 0) endIndex = status.indexOf('>', fsIndex);
        String fsStr = status.substring(fsIndex + 3, endIndex);
        sscanf(fsStr.c_str(), "%d,%d", &fluidnc.feedRate, &fluidnc.spindleRPM);
    }

    // Parse Ov (Overrides) if present
    int ovIndex = status.indexOf("Ov:");
    if (ovIndex >= 0) {
        int endIndex = status.indexOf('|', ovIndex);
        if (endIndex < 0) endIndex = status.indexOf('>', ovIndex);
        String ovStr = status.substring(ovIndex + 3, endIndex);
        sscanf(ovStr.c_str(), "%d,%d,%d", &fluidnc.feedOverride, &fluidnc.rapidOverride, &fluidnc.spindleOverride);
    }
}
