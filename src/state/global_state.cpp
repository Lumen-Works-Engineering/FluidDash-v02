#include "global_state.h"

// ========== HARDWARE INSTANCES ==========
StorageManager storage;
RTC_DS3231 rtc;
WebSocketsClient webSocket;
Preferences prefs;
WebServer server(80);
WiFiManager wm;

// ========== DISPLAY STATE ==========
DisplayMode currentMode;
bool sdCardAvailable = false;

// ========== SENSOR DATA ==========
SensorState sensors = {
    .temperatures = {0},
    .peakTemps = {0},
    .psuVoltage = 0,
    .psuMin = 99.9,
    .psuMax = 0.0,
    .adcSamples = {{0}},
    .adcSampleIndex = 0,
    .adcCurrentSensor = 0,
    .lastAdcSample = 0,
    .adcReady = false,
    .tachCounter = 0,
    .fanRPM = 0,
    .fanSpeed = 0
};

// ========== TEMPERATURE HISTORY ==========
HistoryState history = {
    .tempHistory = nullptr,
    .historySize = 0,
    .historyIndex = 0
};

// ========== FLUIDNC STATE ==========
FluidNCState fluidnc = {
    .machineState = "OFFLINE",
    .posX = 0, .posY = 0, .posZ = 0, .posA = 0,
    .wposX = 0, .wposY = 0, .wposZ = 0, .wposA = 0,
    .wcoX = 0, .wcoY = 0, .wcoZ = 0, .wcoA = 0,
    .feedRate = 0,
    .spindleRPM = 0,
    .feedOverride = 100,
    .rapidOverride = 100,
    .spindleOverride = 100,
    .connected = false,
    .connectionAttempted = false,
    .jobStartTime = 0,
    .isJobRunning = false,
    .autoReportingEnabled = false,
    .reportingSetupTime = 0,
    .debugWebSocket = false
};

// ========== NETWORK STATE ==========
NetworkState network = {
    .inAPMode = false,
    .webServerStarted = false,
    .rtcAvailable = false
};

// ========== TIMING STATE ==========
TimingState timing = {
    .lastTachRead = 0,
    .lastDisplayUpdate = 0,
    .lastHistoryUpdate = 0,
    .lastStatusRequest = 0,
    .sessionStartTime = 0,
    .buttonPressStart = 0,
    .bootCompleteTime = 0,
    .buttonPressed = false
};

void initGlobalState() {
    // Any runtime initialization if needed
    currentMode = MODE_MONITOR;
}
