#include "GPS.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

GPS::GPS(ATClient& at) : _at(at) {}

bool GPS::begin() {
    return true;
}

bool GPS::powerOn() {
    if (_at.sendCmd("AT+CGNSSPWR=1", "OK", AT_DEFAULT_TIMEOUT) != ATResult::OK)
        return false;
    _running = true;
    delay(1000);
    return true;
}

bool GPS::powerOff() {
    if (_at.sendCmd("AT+CGNSSPWR=0") != ATResult::OK) return false;
    _running = false;
    return true;
}

bool GPS::isRunning() {
    return _running;
}

bool GPS::enableNMEA(bool enable) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CGNSSPORTSWITCH=0,%d", enable ? 1 : 0);
    return _at.sendCmd(cmd) == ATResult::OK;
}

bool GPS::injectTime(const char* utcTime) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CGNSSTST=%s", utcTime);
    return _at.sendCmd(cmd) == ATResult::OK;
}

bool GPS::getFix(GNSSFix& fix) {
    char resp[256];
    fix.valid = false;

    if (_at.sendCmd("AT+CGNSSINFO", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) != ATResult::OK)
        return false;

    return _parseGNSSInfo(resp, fix);
}

bool GPS::waitForFix(GNSSFix& fix, uint32_t timeoutMs) {
    uint32_t t0 = millis();
    while (millis() - t0 < timeoutMs) {
        if (getFix(fix) && fix.valid) return true;
        delay(2000);
    }
    return false;
}

bool GPS::_parseGNSSInfo(const char* raw, GNSSFix& fix) {
    // +CGNSSINFO: <mode>,<GPS_sats>,<GLONASS_sats>,<BEIDOU_sats>,<lat>,<N/S>,
    //             <lon>,<E/W>,<date>,<UTC_time>,<alt>,<speed>,<course>,<PDOP>,<HDOP>,<VDOP>
    const char* p = strstr(raw, "+CGNSSINFO:");
    if (!p) return false;
    p += 12;

    char mode[4], gsats[4], rsats[4], bsats[4];
    char latStr[16], ns[2], lonStr[16], ew[2];
    char date[8], time[12];
    float alt, spd, crs, pdop, hdop, vdop;

    int n = sscanf(p, "%3[^,],%3[^,],%3[^,],%3[^,],%15[^,],%1[^,],%15[^,],%1[^,],"
                   "%7[^,],%11[^,],%f,%f,%f,%f,%f,%f",
                   mode, gsats, rsats, bsats, latStr, ns, lonStr, ew,
                   date, time, &alt, &spd, &crs, &pdop, &hdop, &vdop);

    if (n < 10) return false;

    // Empty lat/lon means no fix
    if (latStr[0] == '\0' || latStr[0] == ',') return false;

    // Convert DDMM.MMMM to decimal degrees
    float rawLat = atof(latStr);
    int degLat   = (int)(rawLat / 100);
    fix.latitude  = degLat + (rawLat - degLat * 100) / 60.0f;
    if (ns[0] == 'S') fix.latitude = -fix.latitude;

    float rawLon = atof(lonStr);
    int degLon   = (int)(rawLon / 100);
    fix.longitude = degLon + (rawLon - degLon * 100) / 60.0f;
    if (ew[0] == 'W') fix.longitude = -fix.longitude;

    fix.altitude    = alt;
    fix.speed       = spd;
    fix.course      = crs;
    fix.hdop        = hdop;
    fix.satellites  = atoi(gsats) + atoi(rsats) + atoi(bsats);

    // Build UTC string: date=DDMMYY, time=HHMMSS.sss
    snprintf(fix.utcTime, sizeof(fix.utcTime),
             "20%c%c-%c%c-%c%c %c%c:%c%c:%c%c",
             date[4], date[5], date[2], date[3], date[0], date[1],
             time[0], time[1], time[2], time[3], time[4], time[5]);

    fix.valid = true;
    return true;
}
