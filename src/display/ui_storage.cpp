#include "ui_modes.h"
#include "ui_layout.h"
#include "display.h"
#include "state/global_state.h"
#include "storage_manager.h"
#include "logging/data_logger.h"
#include <SD.h>
#include <LittleFS.h>

extern StorageManager storage;

void drawStorageMode() {
    gfx.fillScreen(TFT_BLACK);
    gfx.setTextColor(TFT_WHITE, TFT_BLACK);

    // Header
    gfx.setTextSize(StorageLayout::HEADER_FONT_SIZE);
    gfx.setCursor(StorageLayout::HEADER_X, StorageLayout::HEADER_Y);
    gfx.print("Storage & Logging");

    gfx.setTextSize(StorageLayout::SECTION_FONT_SIZE);

    // ========== SD Card Section ==========
    gfx.setCursor(StorageLayout::SD_LABEL_X, StorageLayout::SD_LABEL_Y);
    gfx.print("SD Card:");

    gfx.setCursor(StorageLayout::SD_STATUS_X, StorageLayout::SD_STATUS_Y);
    if (storage.isSDAvailable()) {
        gfx.setTextColor(TFT_GREEN, TFT_BLACK);
        gfx.print("OK");

        // Get SD card space
        gfx.setTextColor(TFT_WHITE, TFT_BLACK);
        gfx.setCursor(StorageLayout::SD_SPACE_X, StorageLayout::SD_SPACE_Y);

        uint64_t totalBytes = SD.totalBytes();
        uint64_t usedBytes = SD.usedBytes();
        uint64_t freeBytes = totalBytes - usedBytes;
        float freeGB = freeBytes / (1024.0 * 1024.0 * 1024.0);
        float totalGB = totalBytes / (1024.0 * 1024.0 * 1024.0);

        gfx.printf("  Free: %.1f GB / %.1f GB", freeGB, totalGB);
    } else {
        gfx.setTextColor(TFT_RED, TFT_BLACK);
        gfx.print("NOT DETECTED");
        gfx.setTextColor(TFT_WHITE, TFT_BLACK);
    }

    // ========== SPIFFS Section ==========
    gfx.setCursor(StorageLayout::SPIFFS_LABEL_X, StorageLayout::SPIFFS_LABEL_Y);
    gfx.print("SPIFFS:");

    gfx.setCursor(StorageLayout::SPIFFS_STATUS_X, StorageLayout::SPIFFS_STATUS_Y);
    if (storage.isSPIFFSAvailable()) {
        gfx.setTextColor(TFT_GREEN, TFT_BLACK);
        gfx.print("OK");

        // Get SPIFFS space
        gfx.setTextColor(TFT_WHITE, TFT_BLACK);
        gfx.setCursor(StorageLayout::SPIFFS_SPACE_X, StorageLayout::SPIFFS_SPACE_Y);

        size_t totalBytes = LittleFS.totalBytes();
        size_t usedBytes = LittleFS.usedBytes();
        float freeMB = (totalBytes - usedBytes) / (1024.0 * 1024.0);
        float totalMB = totalBytes / (1024.0 * 1024.0);

        gfx.printf("  Free: %.1f MB / %.1f MB", freeMB, totalMB);
    } else {
        gfx.setTextColor(TFT_RED, TFT_BLACK);
        gfx.print("ERROR");
        gfx.setTextColor(TFT_WHITE, TFT_BLACK);
    }

    // ========== Divider Line ==========
    gfx.drawFastHLine(StorageLayout::DIVIDER_X1, StorageLayout::DIVIDER_Y,
                      StorageLayout::DIVIDER_X2 - StorageLayout::DIVIDER_X1, TFT_DARKGREY);

    // ========== Data Logging Section ==========
    gfx.setTextSize(StorageLayout::LOG_TITLE_FONT_SIZE);
    gfx.setCursor(StorageLayout::LOG_TITLE_X, StorageLayout::LOG_TITLE_Y);
    gfx.print("Data Logging");

    gfx.setTextSize(StorageLayout::VALUE_FONT_SIZE);

    // Logging status
    gfx.setCursor(StorageLayout::LOG_STATUS_LABEL_X, StorageLayout::LOG_STATUS_LABEL_Y);
    gfx.print("Status:");
    gfx.setCursor(StorageLayout::LOG_STATUS_VALUE_X, StorageLayout::LOG_STATUS_VALUE_Y);
    if (logger.isEnabled()) {
        gfx.setTextColor(TFT_GREEN, TFT_BLACK);
        gfx.print("ENABLED");
    } else {
        gfx.setTextColor(TFT_YELLOW, TFT_BLACK);
        gfx.print("DISABLED");
    }
    gfx.setTextColor(TFT_WHITE, TFT_BLACK);

    // Log interval
    gfx.setCursor(StorageLayout::LOG_INTERVAL_LABEL_X, StorageLayout::LOG_INTERVAL_LABEL_Y);
    gfx.print("Interval:");
    gfx.setCursor(StorageLayout::LOG_INTERVAL_VALUE_X, StorageLayout::LOG_INTERVAL_VALUE_Y);
    unsigned long interval = logger.getInterval();
    if (interval >= 60000) {
        gfx.printf("%lu min", interval / 60000);
    } else {
        gfx.printf("%lu sec", interval / 1000);
    }

    // Current log file
    if (logger.isEnabled()) {
        gfx.setCursor(StorageLayout::LOG_FILE_LABEL_X, StorageLayout::LOG_FILE_LABEL_Y);
        gfx.print("Current File:");

        String filename = logger.getCurrentLogFilename();
        // Extract just the filename from the path
        int lastSlash = filename.lastIndexOf('/');
        if (lastSlash >= 0) {
            filename = filename.substring(lastSlash + 1);
        }

        gfx.setCursor(StorageLayout::LOG_FILE_NAME_X, StorageLayout::LOG_FILE_NAME_Y);
        gfx.print(filename);

        // Get file size if it exists
        String fullPath = logger.getCurrentLogFilename();
        if (SD.exists(fullPath.c_str())) {
            File file = SD.open(fullPath.c_str(), FILE_READ);
            if (file) {
                size_t fileSize = file.size();
                file.close();

                gfx.setCursor(StorageLayout::LOG_FILE_SIZE_X, StorageLayout::LOG_FILE_SIZE_Y);
                if (fileSize >= 1024 * 1024) {
                    float sizeMB = fileSize / (1024.0 * 1024.0);
                    gfx.printf("Size: %.2f MB / 10 MB", sizeMB);
                } else {
                    float sizeKB = fileSize / 1024.0;
                    gfx.printf("Size: %.1f KB", sizeKB);
                }
            }
        }
    }

    // Total log file count
    gfx.setCursor(StorageLayout::LOG_COUNT_X, StorageLayout::LOG_COUNT_Y);
    std::vector<String> files = logger.listLogFiles();
    gfx.printf("Total Log Files: %d", files.size());

    // Footer instructions
    gfx.setTextSize(1);
    gfx.setTextColor(TFT_DARKGREY, TFT_BLACK);
    gfx.setCursor(10, 305);
    gfx.print("Tap screen to change modes");
}

void updateStorageMode() {
    // Storage mode is relatively static, so we can update less frequently
    // Only update the log file size if logging is enabled
    if (!logger.isEnabled()) {
        return;  // Nothing dynamic to update when logging is disabled
    }

    // Update current log file size
    gfx.setTextSize(StorageLayout::VALUE_FONT_SIZE);
    gfx.setTextColor(TFT_WHITE, TFT_BLACK);

    String fullPath = logger.getCurrentLogFilename();
    if (SD.exists(fullPath.c_str())) {
        File file = SD.open(fullPath.c_str(), FILE_READ);
        if (file) {
            size_t fileSize = file.size();
            file.close();

            // Clear the size area and redraw
            gfx.fillRect(StorageLayout::LOG_FILE_SIZE_X, StorageLayout::LOG_FILE_SIZE_Y,
                        460, 15, TFT_BLACK);

            gfx.setCursor(StorageLayout::LOG_FILE_SIZE_X, StorageLayout::LOG_FILE_SIZE_Y);
            if (fileSize >= 1024 * 1024) {
                float sizeMB = fileSize / (1024.0 * 1024.0);
                gfx.printf("Size: %.2f MB / 10 MB", sizeMB);
            } else {
                float sizeKB = fileSize / 1024.0;
                gfx.printf("Size: %.1f KB", sizeKB);
            }
        }
    }
}
