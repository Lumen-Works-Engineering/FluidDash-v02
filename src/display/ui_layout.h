#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include <Arduino.h>

// ========== UI LAYOUT CONFIGURATION ==========
// This file contains all hard-coded layout constants for easy editing
// Change font sizes, positions, and spacing in one place instead of
// hunting through 200+ lines of drawing code per screen mode.

// ========== COMMON LAYOUT ==========
namespace CommonLayout {
    constexpr int SCREEN_WIDTH = 480;
    constexpr int SCREEN_HEIGHT = 320;
    constexpr int HEADER_HEIGHT = 25;
    constexpr int HEADER_FONT_SIZE = 2;
}

// ========== MONITOR MODE LAYOUT ==========
namespace MonitorLayout {
    // Header section
    constexpr int HEADER_TITLE_X = 10;
    constexpr int HEADER_TITLE_Y = 6;
    constexpr int HEADER_FONT_SIZE = 2;
    constexpr int DATETIME_X = 270;
    constexpr int DATETIME_Y = 6;
    constexpr int DATETIME_WIDTH = 210;

    // Divider lines
    constexpr int TOP_DIVIDER_Y = 25;
    constexpr int MIDDLE_DIVIDER_Y = 175;
    constexpr int VERTICAL_DIVIDER_X = 240;
    constexpr int VERTICAL_DIVIDER_HEIGHT = 150;

    // Left section - Driver temperatures
    constexpr int TEMP_SECTION_X = 10;
    constexpr int TEMP_LABEL_Y = 30;
    constexpr int TEMP_LABEL_FONT_SIZE = 1;
    constexpr int TEMP_START_Y = 50;
    constexpr int TEMP_ROW_SPACING = 30;
    constexpr int TEMP_LABEL_X = 10;
    constexpr int TEMP_VALUE_X = 50;
    constexpr int TEMP_VALUE_Y_OFFSET = -3;      // Offset from row Y position
    constexpr int TEMP_VALUE_FONT_SIZE = 2;
    constexpr int TEMP_VALUE_WIDTH = 180;        // Width of temp display area to clear
    constexpr int TEMP_VALUE_HEIGHT = 20;        // Height of temp display area to clear
    constexpr int PEAK_TEMP_X = 140;
    constexpr int PEAK_TEMP_Y_OFFSET = 2;        // Offset from row Y position
    constexpr int PEAK_TEMP_FONT_SIZE = 1;

    // Bottom status section
    constexpr int STATUS_LABEL_X = 10;
    constexpr int STATUS_LABEL_Y = 185;
    constexpr int STATUS_LABEL_FONT_SIZE = 1;
    constexpr int STATUS_FAN_Y = 200;
    constexpr int STATUS_PSU_Y = 215;
    constexpr int STATUS_FLUIDNC_Y = 230;
    constexpr int STATUS_COORDS_WCS_Y = 250;
    constexpr int STATUS_COORDS_MCS_Y = 265;
    constexpr int STATUS_VALUE_WIDTH = 220;      // Width of status area to clear
    constexpr int STATUS_VALUE_HEIGHT = 10;      // Height of status line

    // Right section - Temperature history graph
    constexpr int GRAPH_LABEL_X = 250;
    constexpr int GRAPH_LABEL_Y = 30;
    constexpr int GRAPH_TIMESPAN_Y = 40;
    constexpr int GRAPH_X = 250;
    constexpr int GRAPH_Y = 55;
    constexpr int GRAPH_WIDTH = 220;
    constexpr int GRAPH_HEIGHT = 110;
}

// ========== ALIGNMENT MODE LAYOUT ==========
namespace AlignmentLayout {
    // Header
    constexpr int TITLE_X = 140;
    constexpr int TITLE_Y = 6;
    constexpr int TITLE_FONT_SIZE = 2;

    // Subtitle
    constexpr int SUBTITLE_X = 150;
    constexpr int SUBTITLE_Y = 40;
    constexpr int SUBTITLE_FONT_SIZE = 2;

    // 3-axis display (large coordinates)
    constexpr int COORD_3AXIS_START_X = 40;
    constexpr int COORD_3AXIS_START_Y = 90;
    constexpr int COORD_3AXIS_SPACING = 65;
    constexpr int COORD_3AXIS_FONT_SIZE = 5;

    // 4-axis display (compact coordinates)
    constexpr int COORD_4AXIS_START_X = 40;
    constexpr int COORD_4AXIS_START_Y = 75;
    constexpr int COORD_4AXIS_SPACING = 45;
    constexpr int COORD_4AXIS_FONT_SIZE = 4;

    // Machine position footer (4-axis only)
    constexpr int MACHINE_POS_Y = 265;
    constexpr int MACHINE_POS_X = 10;
    constexpr int MACHINE_POS_FONT_SIZE = 1;
}

// ========== GRAPH MODE LAYOUT ==========
namespace GraphLayout {
    // Header
    constexpr int TITLE_X = 100;
    constexpr int TITLE_Y = 6;
    constexpr int TITLE_FONT_SIZE = 2;
    constexpr int TIMESPAN_LABEL_X = 330;
    constexpr int TIMESPAN_LABEL_Y = 10;
    constexpr int TIMESPAN_LABEL_FONT_SIZE = 1;

    // Full-screen graph
    constexpr int GRAPH_X = 20;
    constexpr int GRAPH_Y = 40;
    constexpr int GRAPH_WIDTH = 440;
    constexpr int GRAPH_HEIGHT = 270;
}

// ========== NETWORK MODE LAYOUT ==========
namespace NetworkLayout {
    // Header
    constexpr int TITLE_X = 120;
    constexpr int TITLE_Y = 6;
    constexpr int TITLE_FONT_SIZE = 2;

    // AP Mode display
    constexpr int AP_TITLE_X = 60;
    constexpr int AP_TITLE_Y = 50;
    constexpr int AP_TITLE_FONT_SIZE = 2;

    constexpr int AP_STEP1_X = 10;
    constexpr int AP_STEP1_Y = 90;
    constexpr int AP_SSID_X = 40;
    constexpr int AP_SSID_Y = 110;
    constexpr int AP_SSID_FONT_SIZE = 2;

    constexpr int AP_STEP2_X = 10;
    constexpr int AP_STEP2_Y = 145;
    constexpr int AP_URL_X = 80;
    constexpr int AP_URL_Y = 165;
    constexpr int AP_URL_FONT_SIZE = 2;

    constexpr int AP_STEP3_X = 10;
    constexpr int AP_STEP3_Y = 200;

    constexpr int AP_BACKGROUND_INFO_X = 10;
    constexpr int AP_BACKGROUND_INFO_Y = 230;

    constexpr int AP_EXIT_INFO_X = 10;
    constexpr int AP_EXIT_INFO_Y = 270;
    constexpr int AP_STEP_FONT_SIZE = 1;

    // Normal status display
    constexpr int STATUS_TITLE_X = 130;
    constexpr int STATUS_TITLE_Y = 50;
    constexpr int STATUS_TITLE_FONT_SIZE = 2;

    constexpr int STATUS_LABEL_X = 10;
    constexpr int STATUS_VALUE_X = 80;
    constexpr int STATUS_SSID_Y = 90;
    constexpr int STATUS_IP_Y = 115;
    constexpr int STATUS_SIGNAL_Y = 140;
    constexpr int STATUS_MDNS_Y = 165;
    constexpr int STATUS_FLUIDNC_Y = 190;
    constexpr int STATUS_ROW_FONT_SIZE = 1;

    // Not connected display
    constexpr int NOT_CONNECTED_TITLE_X = 120;
    constexpr int NOT_CONNECTED_TITLE_Y = 50;
    constexpr int NOT_CONNECTED_INFO1_X = 10;
    constexpr int NOT_CONNECTED_INFO1_Y = 100;
    constexpr int NOT_CONNECTED_INFO2_X = 10;
    constexpr int NOT_CONNECTED_INFO2_Y = 130;

    // Instructions footer
    constexpr int INSTRUCTIONS_LINE1_X = 10;
    constexpr int INSTRUCTIONS_LINE1_Y = 250;
    constexpr int INSTRUCTIONS_LINE2_X = 10;
    constexpr int INSTRUCTIONS_LINE2_Y = 265;
    constexpr int INSTRUCTIONS_FONT_SIZE = 1;
}

// Storage mode layout constants
namespace StorageLayout {
    // Header
    constexpr int HEADER_X = 10;
    constexpr int HEADER_Y = 10;
    constexpr int HEADER_FONT_SIZE = 2;

    // SD Card section
    constexpr int SD_LABEL_X = 10;
    constexpr int SD_LABEL_Y = 45;
    constexpr int SD_STATUS_X = 120;
    constexpr int SD_STATUS_Y = 45;
    constexpr int SD_SPACE_X = 10;
    constexpr int SD_SPACE_Y = 65;

    // SPIFFS section
    constexpr int SPIFFS_LABEL_X = 10;
    constexpr int SPIFFS_LABEL_Y = 90;
    constexpr int SPIFFS_STATUS_X = 120;
    constexpr int SPIFFS_STATUS_Y = 90;
    constexpr int SPIFFS_SPACE_X = 10;
    constexpr int SPIFFS_SPACE_Y = 110;

    // Divider line
    constexpr int DIVIDER_Y = 135;
    constexpr int DIVIDER_X1 = 10;
    constexpr int DIVIDER_X2 = 470;

    // Logging section
    constexpr int LOG_TITLE_X = 10;
    constexpr int LOG_TITLE_Y = 145;
    constexpr int LOG_TITLE_FONT_SIZE = 2;

    constexpr int LOG_STATUS_LABEL_X = 10;
    constexpr int LOG_STATUS_LABEL_Y = 175;
    constexpr int LOG_STATUS_VALUE_X = 150;
    constexpr int LOG_STATUS_VALUE_Y = 175;

    constexpr int LOG_INTERVAL_LABEL_X = 10;
    constexpr int LOG_INTERVAL_LABEL_Y = 195;
    constexpr int LOG_INTERVAL_VALUE_X = 150;
    constexpr int LOG_INTERVAL_VALUE_Y = 195;

    constexpr int LOG_FILE_LABEL_X = 10;
    constexpr int LOG_FILE_LABEL_Y = 215;
    constexpr int LOG_FILE_NAME_X = 20;
    constexpr int LOG_FILE_NAME_Y = 235;
    constexpr int LOG_FILE_SIZE_X = 20;
    constexpr int LOG_FILE_SIZE_Y = 255;

    constexpr int LOG_COUNT_X = 10;
    constexpr int LOG_COUNT_Y = 280;

    // Font sizes
    constexpr int SECTION_FONT_SIZE = 1;
    constexpr int VALUE_FONT_SIZE = 1;
}

#endif // UI_LAYOUT_H
