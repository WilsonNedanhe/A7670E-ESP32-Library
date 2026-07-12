#pragma once
#include "ATClient.h"

// Phase 3 — Device Information
// Queries IMEI, IMSI, ICCID, firmware version, model, and SIM state.

struct DeviceInfoData {
    char imei[20];
    char imsi[20];
    char iccid[24];
    char model[32];
    char firmware[48];
    char manufacturer[32];
};

enum class SIMState {
    READY,
    PIN_REQUIRED,
    PUK_REQUIRED,
    NOT_INSERTED,
    UNKNOWN
};

class DeviceInfo {
public:
    explicit DeviceInfo(ATClient& at);

    bool     getDeviceInfo(DeviceInfoData& info);
    bool     getIMEI(char* buf, size_t len);
    bool     getIMSI(char* buf, size_t len);
    bool     getICCID(char* buf, size_t len);
    bool     getFirmware(char* buf, size_t len);
    SIMState getSIMState();
    bool     unlockSIM(const char* pin);

private:
    ATClient& _at;
};
