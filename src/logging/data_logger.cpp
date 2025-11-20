#include "data_logger.h"
#include "state/global_state.h"
#include "sensors/sensors.h"
#include <SD.h>
#include <RTClib.h>
#include <vector>

// Global logger instance
DataLogger logger;

DataLogger::DataLogger()
    : _enabled(false)
    , _logInterval(DEFAULT_INTERVAL)
    , _lastLogTime(0)
    , _currentLogFile("")
{
}

void DataLogger::begin() {
    // Logging disabled by default - user must enable via web interface
    _enabled = false;
    _lastLogTime = millis();
}

void DataLogger::update() {
    if (!_enabled) {
        return;
    }

    unsigned long now = millis();
    if (now - _lastLogTime >= _logInterval) {
        _lastLogTime = now;
        writeLogEntry();
    }
}

void DataLogger::setEnabled(bool enabled) {
    _enabled = enabled;
    if (enabled) {
        ensureLogDirectory();
        _lastLogTime = millis();  // Reset timer
    }
}

void DataLogger::setInterval(unsigned long intervalMs) {
    // Minimum 1 second, maximum 1 hour
    if (intervalMs < 1000) intervalMs = 1000;
    if (intervalMs > 3600000) intervalMs = 3600000;
    _logInterval = intervalMs;
}

String DataLogger::getCurrentLogFilename() {
    if (_currentLogFile.length() == 0) {
        // Generate filename based on current date: fluiddash_YYYYMMDD.csv
        char filename[32];
        snprintf(filename, sizeof(filename), "/logs/fluiddash_%lu.csv", millis() / 86400000);
        _currentLogFile = String(filename);
    }
    return _currentLogFile;
}

void DataLogger::ensureLogDirectory() {
    if (!SD.exists(LOG_DIR)) {
        SD.mkdir(LOG_DIR);
        Serial.println("[LOGGER] Created /logs directory");
    }
}

void DataLogger::writeLogEntry() {
    if (!SD.begin()) {
        Serial.println("[LOGGER] ERROR: SD card not available");
        return;
    }

    String filename = getCurrentLogFilename();
    bool isNewFile = !SD.exists(filename.c_str());

    // Check if we need to rotate
    if (!isNewFile) {
        File existingFile = SD.open(filename.c_str(), FILE_READ);
        if (existingFile && existingFile.size() > MAX_LOG_SIZE) {
            existingFile.close();
            rotateLogFile();
            filename = getCurrentLogFilename();
            isNewFile = true;  // New file after rotation
        } else if (existingFile) {
            existingFile.close();
        }
    }

    // Open file for append
    File logFile = SD.open(filename.c_str(), FILE_APPEND);
    if (!logFile) {
        Serial.print("[LOGGER] ERROR: Failed to open ");
        Serial.println(filename);
        return;
    }

    // Write CSV header if file is new
    if (isNewFile) {
        logFile.println("Timestamp,TempX,TempYL,TempYR,TempZ,PSU_Voltage,Fan_RPM,Fan_Speed,Machine_State,Pos_X,Pos_Y,Pos_Z");
    }

    // Get timestamp from RTC (or fallback to uptime)
    char timestamp[32];
    if (network.rtcAvailable) {
        DateTime now = rtc.now();
        snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
                 now.year(), now.month(), now.day(),
                 now.hour(), now.minute(), now.second());
    } else {
        snprintf(timestamp, sizeof(timestamp), "%lu", millis() / 1000);
    }

    // Write data row
    char logLine[256];
    snprintf(logLine, sizeof(logLine),
        "%s,%.1f,%.1f,%.1f,%.1f,%.2f,%d,%d,%s,%.3f,%.3f,%.3f",
        timestamp,  // RTC timestamp or uptime
        sensors.temperatures[0],
        sensors.temperatures[1],
        sensors.temperatures[2],
        sensors.temperatures[3],
        sensors.psuVoltage,
        sensors.fanRPM,
        sensors.fanSpeed,
        fluidnc.machineState.c_str(),
        fluidnc.posX,
        fluidnc.posY,
        fluidnc.posZ
    );

    logFile.println(logLine);
    logFile.close();
}

void DataLogger::rotateLogFile() {
    // Clear current filename to force new one on next write
    _currentLogFile = "";
    Serial.println("[LOGGER] Log file rotated");
}

bool DataLogger::deleteAllLogs() {
    if (!SD.exists(LOG_DIR)) {
        return true;  // Nothing to delete
    }

    File dir = SD.open(LOG_DIR);
    if (!dir) {
        return false;
    }

    int deletedCount = 0;
    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String filename = String(LOG_DIR) + "/" + String(file.name());
            file.close();
            if (SD.remove(filename.c_str())) {
                deletedCount++;
            }
        } else {
            file.close();
        }
        file = dir.openNextFile();
    }
    dir.close();

    Serial.print("[LOGGER] Deleted ");
    Serial.print(deletedCount);
    Serial.println(" log files");

    _currentLogFile = "";  // Reset current log file
    return true;
}

std::vector<String> DataLogger::listLogFiles() {
    std::vector<String> files;

    if (!SD.exists(LOG_DIR)) {
        return files;
    }

    File dir = SD.open(LOG_DIR);
    if (!dir) {
        return files;
    }

    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            files.push_back(String(file.name()));
        }
        file.close();
        file = dir.openNextFile();
    }
    dir.close();

    return files;
}
