#include "Logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

Logger::Logger(int csPin) : _csPin(csPin) {
    strncpy(_ts, "0000-00-00 00:00:00", sizeof(_ts) - 1);
}

bool Logger::begin() {
    if (!_sd.begin(_csPin, SD_SCK_MHZ(4))) {
        Serial.println("[Logger] SD init failed");
        return false;
    }
    if (!_file.open("/a7670e.log", O_RDWR | O_CREAT | O_APPEND)) {
        Serial.println("[Logger] Failed to open log file");
        return false;
    }
    _ready = true;
    return true;
}

void Logger::end() {
    _file.close();
    _ready = false;
}

void Logger::setTimestamp(const char* utcTimestamp) {
    strncpy(_ts, utcTimestamp, sizeof(_ts) - 1);
}

const char* Logger::_levelStr(LogLevel l) {
    switch (l) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default:              return "INFO";
    }
}

void Logger::log(LogLevel level, const char* tag, const char* msg) {
    char line[256];
    snprintf(line, sizeof(line), "%s,%s,%s,%s\n", _ts, _levelStr(level), tag, msg);
    Serial.print(line);
    if (_ready) {
        _file.print(line);
    }
}

void Logger::logf(LogLevel level, const char* tag, const char* fmt, ...) {
    char msg[200];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    log(level, tag, msg);
}

void Logger::logKV(LogLevel level, const char* tag,
                           const char* key, const char* value) {
    char msg[128];
    snprintf(msg, sizeof(msg), "%s=%s", key, value);
    log(level, tag, msg);
}

void Logger::logJSON(LogLevel level, const char* tag, const char* json) {
    char line[512];
    snprintf(line, sizeof(line), "%s,%s,%s,%s\n", _ts, _levelStr(level), tag, json);
    Serial.print(line);
    if (_ready) {
        _file.print(line);
    }
}

bool Logger::rotate(const char* filename) {
    _file.close();
    if (!_file.open(filename, O_RDWR | O_CREAT | O_TRUNC)) return false;
    return true;
}

void Logger::flush() {
    if (_ready) _file.flush();
}
