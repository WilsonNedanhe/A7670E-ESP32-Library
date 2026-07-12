#include "DeviceInfo.h"
#include <string.h>
#include <stdio.h>

DeviceInfo::DeviceInfo(ATClient& at) : _at(at) {}

static void _extractLine(const char* resp, const char* prefix, char* out, size_t outLen) {
    const char* p = strstr(resp, prefix);
    if (!p) { out[0] = '\0'; return; }
    p += strlen(prefix);
    while (*p == ' ' || *p == ':') p++;
    size_t i = 0;
    while (*p && *p != '\r' && *p != '\n' && i < outLen - 1)
        out[i++] = *p++;
    out[i] = '\0';
}

bool DeviceInfo::getIMEI(char* buf, size_t len) {
    char resp[64];
    if (_at.sendCmd("AT+CGSN", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) != ATResult::OK)
        return false;
    // Response is the IMEI on its own line before OK
    char* p = resp;
    while (*p == '\n' || *p == '\r') p++;
    size_t i = 0;
    while (*p && *p != '\r' && *p != '\n' && i < len - 1)
        buf[i++] = *p++;
    buf[i] = '\0';
    return i > 0;
}

bool DeviceInfo::getIMSI(char* buf, size_t len) {
    char resp[64];
    if (_at.sendCmd("AT+CIMI", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) != ATResult::OK)
        return false;
    char* p = resp;
    while (*p == '\n' || *p == '\r') p++;
    size_t i = 0;
    while (*p && *p != '\r' && *p != '\n' && i < len - 1)
        buf[i++] = *p++;
    buf[i] = '\0';
    return i > 0;
}

bool DeviceInfo::getICCID(char* buf, size_t len) {
    char resp[64];
    if (_at.sendCmd("AT+CICCID", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) != ATResult::OK)
        return false;
    _extractLine(resp, "+ICCID:", buf, len);
    return buf[0] != '\0';
}

bool DeviceInfo::getFirmware(char* buf, size_t len) {
    char resp[128];
    if (_at.sendCmd("AT+CGMR", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) != ATResult::OK)
        return false;
    char* p = resp;
    while (*p == '\n' || *p == '\r') p++;
    size_t i = 0;
    while (*p && *p != '\r' && *p != '\n' && i < len - 1)
        buf[i++] = *p++;
    buf[i] = '\0';
    return i > 0;
}

bool DeviceInfo::getDeviceInfo(DeviceInfoData& info) {
    bool ok = true;
    ok &= getIMEI    (info.imei,         sizeof(info.imei));
    ok &= getIMSI    (info.imsi,         sizeof(info.imsi));
    ok &= getICCID   (info.iccid,        sizeof(info.iccid));
    ok &= getFirmware(info.firmware,     sizeof(info.firmware));

    char resp[64];
    if (_at.sendCmd("AT+CGMM", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) == ATResult::OK) {
        char* p = resp; while (*p == '\n' || *p == '\r') p++;
        strncpy(info.model, p, sizeof(info.model) - 1);
    }
    if (_at.sendCmd("AT+CGMI", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) == ATResult::OK) {
        char* p = resp; while (*p == '\n' || *p == '\r') p++;
        strncpy(info.manufacturer, p, sizeof(info.manufacturer) - 1);
    }
    return ok;
}

SIMState DeviceInfo::getSIMState() {
    char resp[64];
    if (_at.sendCmd("AT+CPIN?", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) != ATResult::OK)
        return SIMState::UNKNOWN;

    if (strstr(resp, "READY"))       return SIMState::READY;
    if (strstr(resp, "SIM PIN"))     return SIMState::PIN_REQUIRED;
    if (strstr(resp, "SIM PUK"))     return SIMState::PUK_REQUIRED;
    if (strstr(resp, "NOT INSERTED")) return SIMState::NOT_INSERTED;
    return SIMState::UNKNOWN;
}

bool DeviceInfo::unlockSIM(const char* pin) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CPIN=\"%s\"", pin);
    return _at.sendCmd(cmd) == ATResult::OK;
}
