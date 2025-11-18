#include "sensors.h"
#include "config/pins.h"
#include "config/config.h"
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ========== DS18B20 OneWire Setup ==========
OneWire oneWire(ONE_WIRE_BUS_1);
DallasTemperature ds18b20Sensors(&oneWire);

// Sensor mappings vector (stores UID to friendly name mappings)
std::vector<SensorMapping> sensorMappings;

// ========== Temperature Monitoring ==========

// Legacy function - now just calls non-blocking version
// Kept for compatibility but processing now happens in loop via non-blocking functions
void readTemperatures() {
  // This function kept for compatibility but processing now happens in loop
}

// Calculate temperature from thermistor ADC value using Steinhart-Hart equation
// This is legacy code for thermistor-based temperature sensing
// CYD uses DS18B20 OneWire sensors instead
float calculateThermistorTemp(float adcValue) {
  float voltage = (adcValue / ADC_RESOLUTION) * 3.3;
  if (voltage <= 0.01) return 0.0;  // Prevent division by zero

  float resistance = SERIES_RESISTOR * (3.3 / voltage - 1.0);
  float steinhart = resistance / THERMISTOR_NOMINAL;
  steinhart = log(steinhart);
  steinhart /= B_COEFFICIENT;
  steinhart += 1.0 / (TEMPERATURE_NOMINAL + 273.15);
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;
  return steinhart;
}

// Update temperature history buffer with max temperature
void updateTempHistory() {
  float maxTemp = temperatures[0];
  for (int i = 1; i < 4; i++) {
    if (temperatures[i] > maxTemp) {
      maxTemp = temperatures[i];
    }
  }

  tempHistory[historyIndex] = maxTemp;
  historyIndex = (historyIndex + 1) % historySize;
}

// ========== Fan Control ==========

// Control fan speed based on maximum temperature
// Maps temperature between low and high thresholds to fan speed range
void controlFan() {
  float maxTemp = temperatures[0];
  for (int i = 1; i < 4; i++) {
    if (temperatures[i] > maxTemp) {
      maxTemp = temperatures[i];
    }
  }

  if (maxTemp < cfg.temp_threshold_low) {
    fanSpeed = cfg.fan_min_speed;
  } else if (maxTemp > cfg.temp_threshold_high) {
    fanSpeed = cfg.fan_max_speed_limit;
  } else {
    fanSpeed = map(maxTemp * 100, cfg.temp_threshold_low * 100,
                   cfg.temp_threshold_high * 100,
                   cfg.fan_min_speed, cfg.fan_max_speed_limit);
  }

  uint8_t pwmValue = map(fanSpeed, 0, 100, 0, 255);
  ledcWrite(0, pwmValue);  // channel 0
}

// Calculate fan RPM from tachometer pulses
// Assumes tachISR() is incrementing tachCounter on each pulse
// Most fans output 2 pulses per revolution
void calculateRPM() {
  fanRPM = (tachCounter * 60) / 2;
  tachCounter = 0;
}

// Tachometer interrupt handler (IRAM for fast execution)
// Defined in main.cpp - this declaration is here for reference
// void IRAM_ATTR tachISR() {
//   tachCounter++;
// }

// ========== PSU Monitoring ==========

// Non-blocking sensor sampling - call this repeatedly in loop()
// Samples PSU voltage ADC every 5ms and averages 10 samples
void sampleSensorsNonBlocking() {
  if (millis() - lastAdcSample < 5) {
    return;  // Sample every 5ms
  }
  lastAdcSample = millis();

  // CYD NOTE: Only PSU voltage is ADC-based now (temperatures use DS18B20 OneWire)
  // Take one sample from PSU voltage sensor only
  adcSamples[4][adcSampleIndex] = analogRead(PSU_VOLT);  // Index 4 = PSU voltage

  // Move to next sample
  adcSampleIndex++;
  if (adcSampleIndex >= 10) {
    adcSampleIndex = 0;
    adcReady = true;  // PSU voltage sampling complete
  }
}

// Process averaged ADC readings (called when adcReady is true)
// Calculates PSU voltage from averaged ADC samples and reads DS18B20 sensors
void processAdcReadings() {
  // Read DS18B20 temperature sensors
  ds18b20Sensors.requestTemperatures();  // Request readings from all sensors

  // Update temperatures array with readings from mapped sensors
  int deviceCount = ds18b20Sensors.getDeviceCount();

  // Clear temperatures array first
  for (int i = 0; i < 4; i++) {
    temperatures[i] = 0.0;
  }

  // Read temperatures based on sensor mappings
  for (size_t i = 0; i < sensorMappings.size() && i < 4; i++) {
    if (sensorMappings[i].enabled) {
      float temp = ds18b20Sensors.getTempC(sensorMappings[i].uid);
      if (temp != DEVICE_DISCONNECTED_C && temp > -55.0 && temp < 125.0) {
        temperatures[i] = temp;
        if (temp > peakTemps[i]) {
          peakTemps[i] = temp;
        }
      }
    }
  }

  // If no mappings exist yet, read first 4 discovered sensors directly
  if (sensorMappings.empty() && deviceCount > 0) {
    for (int i = 0; i < min(deviceCount, 4); i++) {
      float temp = ds18b20Sensors.getTempCByIndex(i);
      if (temp != DEVICE_DISCONNECTED_C && temp > -55.0 && temp < 125.0) {
        temperatures[i] = temp;
        if (temp > peakTemps[i]) {
          peakTemps[i] = temp;
        }
      }
    }
  }

  // Process PSU voltage
  uint32_t sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += adcSamples[4][i];
  }
  float adcValue = sum / 10.0;
  float measuredVoltage = (adcValue / ADC_RESOLUTION) * 3.3;
  psuVoltage = measuredVoltage * cfg.psu_voltage_cal;

  if (psuVoltage < psuMin && psuVoltage > 10.0) psuMin = psuVoltage;
  if (psuVoltage > psuMax) psuMax = psuVoltage;
}

// ========== Sensor Management Functions ==========

// Initialize DS18B20 sensors on OneWire bus
void initDS18B20Sensors() {
  Serial.println("[SENSORS] Initializing DS18B20 sensors...");

  ds18b20Sensors.begin();
  int deviceCount = ds18b20Sensors.getDeviceCount();

  Serial.printf("[SENSORS] Found %d DS18B20 sensor(s) on bus\n", deviceCount);

  // Set resolution to 12-bit for all sensors (0.0625°C precision)
  ds18b20Sensors.setResolution(12);

  // Set wait for conversion to false for non-blocking operation
  ds18b20Sensors.setWaitForConversion(false);

  // Print discovered sensor UIDs
  for (int i = 0; i < deviceCount; i++) {
    uint8_t addr[8];
    if (ds18b20Sensors.getAddress(addr, i)) {
      Serial.printf("[SENSORS] Sensor %d UID: ", i);
      for (int j = 0; j < 8; j++) {
        Serial.printf("%02X", addr[j]);
        if (j < 7) Serial.print(":");
      }
      Serial.println();
    }
  }

  Serial.println("[SENSORS] DS18B20 initialization complete");
}

// Get sensor count from mappings
int getSensorCount() {
  return sensorMappings.size();
}

// Get temperature by alias (e.g., "temp0")
float getTempByAlias(const char* alias) {
  for (const auto& mapping : sensorMappings) {
    if (strcmp(mapping.alias, alias) == 0 && mapping.enabled) {
      float temp = ds18b20Sensors.getTempC(mapping.uid);
      if (temp != DEVICE_DISCONNECTED_C && temp > -55.0 && temp < 125.0) {
        return temp;
      }
    }
  }
  return NAN;  // Return NaN if sensor not found or invalid reading
}

// Get temperature by UID
float getTempByUID(const uint8_t uid[8]) {
  float temp = ds18b20Sensors.getTempC(uid);
  if (temp != DEVICE_DISCONNECTED_C && temp > -55.0 && temp < 125.0) {
    return temp;
  }
  return NAN;
}

// ========== UID Discovery & Conversion Functions ==========

// Discover all DS18B20 sensors on the OneWire bus
// Returns vector of UID strings in format "28FF641E8C160450"
std::vector<String> getDiscoveredUIDs() {
  std::vector<String> uids;
  uint8_t addr[8];

  Serial.println("[SENSORS] Scanning OneWire bus for DS18B20 sensors...");

  oneWire.reset_search();
  while (oneWire.search(addr)) {
    // Verify CRC
    if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("[SENSORS] CRC check failed for sensor");
      continue;
    }

    // Check for DS18B20 family code (0x28)
    if (addr[0] != 0x28) {
      Serial.printf("[SENSORS] Not a DS18B20 sensor (family code: 0x%02X)\n", addr[0]);
      continue;
    }

    String uidStr = uidToString(addr);
    uids.push_back(uidStr);
    Serial.printf("[SENSORS] Found sensor: %s\n", uidStr.c_str());
  }

  Serial.printf("[SENSORS] Discovery complete: %d sensor(s) found\n", uids.size());
  return uids;
}

// Convert UID byte array to hex string
// Input: {0x28, 0xFF, 0x64, 0x1E, 0x8C, 0x16, 0x04, 0x50}
// Output: "28FF641E8C160450"
String uidToString(const uint8_t uid[8]) {
  String result = "";
  for (int i = 0; i < 8; i++) {
    if (uid[i] < 16) result += "0";
    result += String(uid[i], HEX);
  }
  result.toUpperCase();
  return result;
}

// Convert hex string to UID byte array
// Input: "28FF641E8C160450"
// Output: {0x28, 0xFF, 0x64, 0x1E, 0x8C, 0x16, 0x04, 0x50}
void stringToUID(const String& str, uint8_t uid[8]) {
  for (int i = 0; i < 8; i++) {
    String byteStr = str.substring(i * 2, i * 2 + 2);
    uid[i] = strtoul(byteStr.c_str(), NULL, 16);
  }
}

// ========== Sensor Configuration Persistence (NVS) ==========

#include <Preferences.h>
extern Preferences prefs;  // Defined in main.cpp

// Load sensor configuration from NVS
// Stored as: sensor0_uid, sensor0_name, sensor0_alias, sensor0_enabled, sensor0_notes
//            sensor1_uid, sensor1_name, sensor1_alias, sensor1_enabled, sensor1_notes, etc.
void loadSensorConfig() {
  Serial.println("[SENSORS] Loading sensor configuration from NVS...");

  prefs.begin("sensors", true);  // Read-only mode

  // Clear existing mappings
  sensorMappings.clear();

  // Load up to 10 sensor mappings (generous limit)
  for (int i = 0; i < 10; i++) {
    String prefix = "s" + String(i) + "_";

    // Check if this sensor config exists
    String uidKey = prefix + "uid";
    if (!prefs.isKey(uidKey.c_str())) {
      break;  // No more sensors configured
    }

    SensorMapping mapping;

    // Load UID (stored as 16-character hex string)
    String uidStr = prefs.getString(uidKey.c_str(), "");
    if (uidStr.length() == 16) {
      stringToUID(uidStr, mapping.uid);
    } else {
      Serial.printf("[SENSORS] Invalid UID for sensor %d, skipping\n", i);
      continue;
    }

    // Load friendly name
    String nameKey = prefix + "name";
    strlcpy(mapping.friendlyName, prefs.getString(nameKey.c_str(), "").c_str(), sizeof(mapping.friendlyName));

    // Load alias
    String aliasKey = prefix + "alias";
    strlcpy(mapping.alias, prefs.getString(aliasKey.c_str(), ("temp" + String(i)).c_str()).c_str(), sizeof(mapping.alias));

    // Load enabled flag
    String enabledKey = prefix + "en";
    mapping.enabled = prefs.getBool(enabledKey.c_str(), true);

    // Load notes
    String notesKey = prefix + "notes";
    strlcpy(mapping.notes, prefs.getString(notesKey.c_str(), "").c_str(), sizeof(mapping.notes));

    // Load display position
    String posKey = prefix + "pos";
    mapping.displayPosition = prefs.getChar(posKey.c_str(), -1);  // Default -1 = not displayed

    sensorMappings.push_back(mapping);
    Serial.printf("[SENSORS] Loaded: %s -> %s (pos:%d, %s)\n", mapping.alias, mapping.friendlyName, mapping.displayPosition, uidToString(mapping.uid).c_str());
  }

  prefs.end();

  Serial.printf("[SENSORS] Loaded %d sensor mapping(s)\n", sensorMappings.size());
}

// Save sensor configuration to NVS
void saveSensorConfig() {
  Serial.println("[SENSORS] Saving sensor configuration to NVS...");

  prefs.begin("sensors", false);  // Read-write mode

  // Clear all existing sensor keys first
  prefs.clear();

  // Save each sensor mapping
  for (size_t i = 0; i < sensorMappings.size(); i++) {
    String prefix = "s" + String(i) + "_";
    const SensorMapping& mapping = sensorMappings[i];

    // Save UID as hex string
    String uidStr = uidToString(mapping.uid);
    prefs.putString((prefix + "uid").c_str(), uidStr);

    // Save friendly name
    prefs.putString((prefix + "name").c_str(), mapping.friendlyName);

    // Save alias
    prefs.putString((prefix + "alias").c_str(), mapping.alias);

    // Save enabled flag
    prefs.putBool((prefix + "en").c_str(), mapping.enabled);

    // Save notes
    prefs.putString((prefix + "notes").c_str(), mapping.notes);

    // Save display position
    prefs.putChar((prefix + "pos").c_str(), mapping.displayPosition);

    Serial.printf("[SENSORS] Saved: %s -> %s (pos:%d, %s)\n", mapping.alias, mapping.friendlyName, mapping.displayPosition, uidStr.c_str());
  }

  prefs.end();

  Serial.printf("[SENSORS] Saved %d sensor mapping(s)\n", sensorMappings.size());
}

// Add or update sensor mapping
// If UID already exists, update it. Otherwise, add new mapping.
bool addSensorMapping(const uint8_t uid[8], const char* name, const char* alias) {
  // Check if sensor with this UID already exists
  for (auto& mapping : sensorMappings) {
    if (memcmp(mapping.uid, uid, 8) == 0) {
      // Update existing mapping
      strlcpy(mapping.friendlyName, name, sizeof(mapping.friendlyName));
      strlcpy(mapping.alias, alias, sizeof(mapping.alias));
      Serial.printf("[SENSORS] Updated mapping: %s -> %s\n", alias, name);
      saveSensorConfig();
      return true;
    }
  }

  // Add new mapping
  SensorMapping newMapping;
  memcpy(newMapping.uid, uid, 8);
  strlcpy(newMapping.friendlyName, name, sizeof(newMapping.friendlyName));
  strlcpy(newMapping.alias, alias, sizeof(newMapping.alias));
  newMapping.enabled = true;
  newMapping.notes[0] = '\0';
  newMapping.displayPosition = -1;  // Not assigned to display by default

  sensorMappings.push_back(newMapping);
  Serial.printf("[SENSORS] Added mapping: %s -> %s\n", alias, name);
  saveSensorConfig();
  return true;
}

// Remove sensor mapping by alias
bool removeSensorMapping(const char* alias) {
  for (auto it = sensorMappings.begin(); it != sensorMappings.end(); ++it) {
    if (strcmp(it->alias, alias) == 0) {
      Serial.printf("[SENSORS] Removed mapping: %s\n", alias);
      sensorMappings.erase(it);
      saveSensorConfig();
      return true;
    }
  }
  Serial.printf("[SENSORS] Mapping not found: %s\n", alias);
  return false;
}

// Detect which sensor is being touched (temperature rise detection)
// Monitors all sensors for temperature increase above threshold
// Returns UID string of first sensor that shows temperature rise
String detectTouchedSensor(unsigned long timeoutMs, float thresholdDelta) {
  Serial.printf("[SENSORS] Starting touch detection (timeout: %lums, threshold: %.1f°C)\n", timeoutMs, thresholdDelta);

  // Get baseline temperatures for all discovered sensors
  std::vector<String> uids = getDiscoveredUIDs();
  std::vector<float> baselines;

  Serial.println("[SENSORS] Establishing temperature baselines...");
  ds18b20Sensors.requestTemperatures();
  delay(750);  // Wait for conversion (12-bit resolution takes 750ms)

  for (const String& uidStr : uids) {
    uint8_t uid[8];
    stringToUID(uidStr, uid);
    float temp = getTempByUID(uid);
    baselines.push_back(temp);
    Serial.printf("[SENSORS] Baseline for %s: %.2f°C\n", uidStr.c_str(), temp);
  }

  unsigned long startTime = millis();

  Serial.println("[SENSORS] Monitoring for temperature changes... (touch a sensor)");

  while (millis() - startTime < timeoutMs) {
    ds18b20Sensors.requestTemperatures();
    delay(750);  // Wait for conversion
    yield();

    // Check each sensor for temperature rise
    for (size_t i = 0; i < uids.size(); i++) {
      uint8_t uid[8];
      stringToUID(uids[i], uid);
      float currentTemp = getTempByUID(uid);
      float delta = currentTemp - baselines[i];

      if (delta >= thresholdDelta) {
        Serial.printf("[SENSORS] Touch detected! Sensor %s increased by %.2f°C\n", uids[i].c_str(), delta);
        return uids[i];
      }
    }
  }

  Serial.println("[SENSORS] Touch detection timed out - no sensor touched");
  return "";  // Timeout - no sensor touched
}

// ========== Driver Position Management ==========

// Assign sensor UID to a display position (0=X, 1=YL, 2=YR, 3=Z)
// First clears any existing sensor at that position
bool assignSensorToPosition(const uint8_t uid[8], int8_t position) {
  // Clear any sensor currently at this position
  for (auto& mapping : sensorMappings) {
    if (mapping.displayPosition == position) {
      mapping.displayPosition = -1;  // Unassign
      Serial.printf("[SENSORS] Cleared position %d\n", position);
    }
  }

  // Find sensor with this UID and assign to position
  for (auto& mapping : sensorMappings) {
    if (memcmp(mapping.uid, uid, 8) == 0) {
      mapping.displayPosition = position;

      // Auto-assign friendly name if not set
      if (mapping.friendlyName[0] == '\0') {
        const char* names[] = {"X-Axis", "Y-Left", "Y-Right", "Z-Axis"};
        if (position >= 0 && position <= 3) {
          strlcpy(mapping.friendlyName, names[position], sizeof(mapping.friendlyName));
        }
      }

      Serial.printf("[SENSORS] Assigned %s to position %d (%s)\n",
                    uidToString(uid).c_str(), position, mapping.friendlyName);
      saveSensorConfig();
      return true;
    }
  }

  // Sensor not in mappings yet - add it
  SensorMapping newMapping;
  memcpy(newMapping.uid, uid, 8);
  newMapping.displayPosition = position;
  newMapping.enabled = true;
  newMapping.notes[0] = '\0';

  // Auto-assign friendly name and alias
  const char* names[] = {"X-Axis", "Y-Left", "Y-Right", "Z-Axis"};
  const char* aliases[] = {"temp0", "temp1", "temp2", "temp3"};
  if (position >= 0 && position <= 3) {
    strlcpy(newMapping.friendlyName, names[position], sizeof(newMapping.friendlyName));
    strlcpy(newMapping.alias, aliases[position], sizeof(newMapping.alias));
  } else {
    snprintf(newMapping.alias, sizeof(newMapping.alias), "temp%d", position);
    newMapping.friendlyName[0] = '\0';
  }

  sensorMappings.push_back(newMapping);
  Serial.printf("[SENSORS] Added new sensor %s at position %d\n", uidToString(uid).c_str(), position);
  saveSensorConfig();
  return true;
}

// Get sensor UID assigned to a display position
bool getSensorAtPosition(int8_t position, uint8_t uid[8]) {
  for (const auto& mapping : sensorMappings) {
    if (mapping.displayPosition == position && mapping.enabled) {
      memcpy(uid, mapping.uid, 8);
      return true;
    }
  }
  return false;
}

// Get sensor mapping by display position
const SensorMapping* getSensorMappingByPosition(int8_t position) {
  for (const auto& mapping : sensorMappings) {
    if (mapping.displayPosition == position && mapping.enabled) {
      return &mapping;
    }
  }
  return nullptr;
}
