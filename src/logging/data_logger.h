#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <Arduino.h>
#include <SD.h>
#include <vector>

class DataLogger {
public:
    DataLogger();

    void begin();
    void update();  // Call from loop()
    void setEnabled(bool enabled);
    void setInterval(unsigned long intervalMs);

    bool isEnabled() { return _enabled; }
    unsigned long getInterval() { return _logInterval; }
    String getCurrentLogFilename();
    bool deleteAllLogs();
    std::vector<String> listLogFiles();

private:
    void writeLogEntry();
    void rotateLogFile();
    void ensureLogDirectory();

    bool _enabled;
    unsigned long _logInterval;
    unsigned long _lastLogTime;
    String _currentLogFile;

    static constexpr unsigned long DEFAULT_INTERVAL = 10000;  // 10 seconds
    static constexpr size_t MAX_LOG_SIZE = 10 * 1024 * 1024;   // 10 MB
    static constexpr const char* LOG_DIR = "/logs";
};

// Global logger instance
extern DataLogger logger;

#endif // DATA_LOGGER_H
