#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include <Arduino.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <WebSocketsClient.h>
#include <Preferences.h>
#include <RTClib.h>
#include "storage_manager.h"
#include "../config/config.h"

// ========== HARDWARE INSTANCES ==========
extern StorageManager storage;
extern RTC_DS3231 rtc;
extern WebSocketsClient webSocket;
extern Preferences prefs;
extern WebServer server;
extern WiFiManager wm;

// ========== DISPLAY STATE ==========
extern DisplayMode currentMode; 
extern bool sdCardAvailable;
// ========== SENSOR DATA ==========
struct SensorState {
    float temperatures[4];
    float peakTemps[4];
    float psuVoltage;
    float psuMin;
    float psuMax;
    
    // ADC sampling
    uint32_t adcSamples[5][10];
    uint8_t adcSampleIndex;
    uint8_t adcCurrentSensor;
    unsigned long lastAdcSample;
    bool adcReady;
    
    // Fan control
    volatile uint16_t tachCounter;
    uint16_t fanRPM;
    uint8_t fanSpeed;
};
extern SensorState sensors;

// ========== TEMPERATURE HISTORY ==========
struct HistoryState {
    float *tempHistory;
    uint16_t historySize;
    uint16_t historyIndex;
};
extern HistoryState history;

// ========== FLUIDNC STATE ==========
struct FluidNCState {
    String machineState;
    float posX, posY, posZ, posA;
    float wposX, wposY, wposZ, wposA;
    float wcoX, wcoY, wcoZ, wcoA;
    int feedRate;
    int spindleRPM;
    int feedOverride;
    int rapidOverride;
    int spindleOverride;
    bool connected;
    bool connectionAttempted;
    unsigned long jobStartTime;
    bool isJobRunning;
    bool autoReportingEnabled;
    unsigned long reportingSetupTime;
    bool debugWebSocket;
};
extern FluidNCState fluidnc;

// ========== NETWORK STATE ==========
struct NetworkState {
    bool inAPMode;
    bool webServerStarted;
    bool rtcAvailable;
};
extern NetworkState network;

// ========== TIMING STATE ==========
struct TimingState {
    unsigned long lastTachRead;
    unsigned long lastDisplayUpdate;
    unsigned long lastHistoryUpdate;
    unsigned long lastStatusRequest;
    unsigned long sessionStartTime;
    unsigned long buttonPressStart;
    unsigned long bootCompleteTime;
    bool buttonPressed;
};
extern TimingState timing;

// ========== INITIALIZATION ==========
void initGlobalState();

#endif // GLOBAL_STATE_H
