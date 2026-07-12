#pragma once
#include "ATClient.h"

// Phase 7 — GNSS (GPS / GLONASS / BeiDou via A7670E built-in GNSS)

struct GNSSFix {
    bool    valid;
    float   latitude;       // decimal degrees, negative = South
    float   longitude;      // decimal degrees, negative = West
    float   altitude;       // metres MSL
    float   speed;          // km/h
    float   course;         // degrees true North
    float   hdop;
    int     satellites;
    char    utcTime[24];    // "YYYY-MM-DD HH:MM:SS.sss"
};

class GPS {
public:
    explicit GPS(ATClient& at);

    bool begin();
    bool powerOn();
    bool powerOff();

    // Returns true when a valid fix is available
    bool     getFix(GNSSFix& fix);
    bool     waitForFix(GNSSFix& fix, uint32_t timeoutMs = AT_GPS_TIMEOUT);

    // Enable NMEA sentence output on GNSS port
    bool     enableNMEA(bool enable);

    // Assisted GPS (A-GPS) time injection
    bool     injectTime(const char* utcTime);

    bool     isRunning();

private:
    ATClient& _at;
    bool _running = false;

    bool _parseGNSSInfo(const char* raw, GNSSFix& fix);
};
