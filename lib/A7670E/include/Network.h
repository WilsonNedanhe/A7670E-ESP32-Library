#pragma once
#include "ATClient.h"

// Phase 4 — Network Registration & LTE Data
// Handles APN setup, PDP context, registration status, signal quality.

enum class RegStatus {
    NOT_REGISTERED,
    REGISTERED_HOME,
    SEARCHING,
    DENIED,
    UNKNOWN,
    REGISTERED_ROAMING
};

enum class NetTech {
    GSM, EDGE, UMTS, HSDPA, LTE, LTE_CAT_M, NB_IOT, UNKNOWN
};

struct SignalInfo {
    int    rssi;        // dBm
    int    rsrp;        // dBm (LTE)
    int    rsrq;        // dB  (LTE)
    int    sinr;        // dB  (LTE)
    int    csq;         // raw AT+CSQ value 0-31
    NetTech tech;
    char   operatorName[32];
    char   mcc[4];
    char   mnc[4];
    int    lac;
    int    cellId;
};

class Network {
public:
    explicit Network(ATClient& at);

    bool        setAPN(const char* apn,
                       const char* user = "",
                       const char* pass = "");
    bool        activate(uint32_t timeoutMs = AT_NETWORK_TIMEOUT);
    bool        deactivate();
    bool        isConnected();
    RegStatus   getRegistrationStatus();
    bool        getSignalInfo(SignalInfo& info);
    bool        getLocalIP(char* buf, size_t len);
    bool        ping(const char* host, uint32_t timeoutMs = 10000);

    // Preferred radio access technology (e.g., LTE-only = "38")
    bool        setPreferredMode(const char* mode);

private:
    ATClient& _at;
    char  _apn[64];
    char  _user[32];
    char  _pass[32];
};
