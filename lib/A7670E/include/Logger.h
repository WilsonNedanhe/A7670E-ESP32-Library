#pragma once
#include "Config.h"
#include <SdFat.h>

// Phase 10 — SD Card Logger
// Writes timestamped CSV / JSON log entries to SD via SPI.

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger {
public:
    // csPin: SD chip-select GPIO
    explicit Logger(int csPin = 5);

    bool begin();
    void end();

    // Set current timestamp source (call each time GPS fix updates)
    void setTimestamp(const char* utcTimestamp);

    // Log a plain text line
    void log(LogLevel level, const char* tag, const char* msg);
    void logf(LogLevel level, const char* tag, const char* fmt, ...);

    // Log a key-value pair as CSV row:  timestamp,level,tag,key,value
    void logKV(LogLevel level, const char* tag,
               const char* key, const char* value);

    // Log a full JSON object string
    void logJSON(LogLevel level, const char* tag, const char* json);

    // Rotate to a new file (e.g., daily)
    bool rotate(const char* filename);

    // Flush buffers
    void flush();

    bool isReady() const { return _ready; }

private:
    SdFat  _sd;
    FsFile _file;
    int    _csPin;
    bool   _ready = false;
    char   _ts[32];

    const char* _levelStr(LogLevel l);
};
