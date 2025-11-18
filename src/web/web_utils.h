#ifndef WEB_UTILS_H
#define WEB_UTILS_H
#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>

static bool initLittleFS(bool formatOnFail = true) {
    if (!LittleFS.begin(formatOnFail)) {
        return false;
    }
    return true;
}

static String loadFile(const char* path) {
    File file = LittleFS.open(path, "r");
    String contents = file.readString();
    file.close();
    return contents;
}

#endif