#include "Network.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

Network::Network(ATClient& at) : _at(at) {
    memset(_apn, 0, sizeof(_apn));
    memset(_user, 0, sizeof(_user));
    memset(_pass, 0, sizeof(_pass));
}

bool Network::setAPN(const char* apn, const char* user, const char* pass) {
    strncpy(_apn,  apn,  sizeof(_apn)  - 1);
    strncpy(_user, user, sizeof(_user) - 1);
    strncpy(_pass, pass, sizeof(_pass) - 1);
    return true;
}

bool Network::activate(uint32_t timeoutMs) {
    // Configure PDP context 1
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"", _apn);
    if (_at.sendCmd(cmd) != ATResult::OK) return false;

    // Activate
    if (_at.sendCmd("AT+CGACT=1,1", "OK", timeoutMs) != ATResult::OK) return false;

    // If credentials required
    if (_user[0] && _pass[0]) {
        snprintf(cmd, sizeof(cmd),
                 "AT+CGAUTH=1,1,\"%s\",\"%s\"", _user, _pass);
        _at.sendCmd(cmd);
    }
    return true;
}

bool Network::deactivate() {
    return _at.sendCmd("AT+CGACT=0,1") == ATResult::OK;
}

bool Network::isConnected() {
    char resp[64];
    if (_at.sendCmd("AT+CGACT?", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) != ATResult::OK)
        return false;
    // +CGACT: 1,1  →  context 1 is active
    return strstr(resp, "1,1") != nullptr;
}

RegStatus Network::getRegistrationStatus() {
    char resp[64];
    if (_at.sendCmd("AT+CEREG?", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) != ATResult::OK)
        return RegStatus::UNKNOWN;

    // +CEREG: <n>,<stat>
    const char* p = strstr(resp, "+CEREG:");
    if (!p) return RegStatus::UNKNOWN;
    p = strchr(p, ',');
    if (!p) return RegStatus::UNKNOWN;
    int stat = atoi(p + 1);
    switch (stat) {
        case 0: return RegStatus::NOT_REGISTERED;
        case 1: return RegStatus::REGISTERED_HOME;
        case 2: return RegStatus::SEARCHING;
        case 3: return RegStatus::DENIED;
        case 5: return RegStatus::REGISTERED_ROAMING;
        default: return RegStatus::UNKNOWN;
    }
}

bool Network::getSignalInfo(SignalInfo& info) {
    char resp[256];
    memset(&info, 0, sizeof(info));

    // CSQ
    if (_at.sendCmd("AT+CSQ", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) == ATResult::OK) {
        const char* p = strstr(resp, "+CSQ:");
        if (p) {
            info.csq  = atoi(p + 5);
            info.rssi = (info.csq == 99) ? -113 : -113 + info.csq * 2;
        }
    }

    // CEREG with location info (n=2 mode)
    _at.sendCmd("AT+CEREG=2");
    if (_at.sendCmd("AT+CEREG?", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) == ATResult::OK) {
        // +CEREG: 2,<stat>,<tac>,<ci>,<AcT>
        const char* p = strstr(resp, "+CEREG:");
        if (p) {
            int n, stat, act;
            char tac[8], ci[10];
            if (sscanf(p, "+CEREG: %d,%d,\"%6[^\"]\".\"%8[^\"]\",%d",
                       &n, &stat, tac, ci, &act) >= 4) {
                info.lac    = (int)strtol(tac, nullptr, 16);
                info.cellId = (int)strtol(ci,  nullptr, 16);
            }
        }
    }

    // Operator name
    if (_at.sendCmd("AT+COPS?", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) == ATResult::OK) {
        const char* p = strstr(resp, "+COPS:");
        if (p) {
            const char* q = strchr(p, '"');
            if (q) {
                q++;
                size_t i = 0;
                while (*q && *q != '"' && i < sizeof(info.operatorName) - 1)
                    info.operatorName[i++] = *q++;
                info.operatorName[i] = '\0';
            }
        }
    }
    return true;
}

bool Network::getLocalIP(char* buf, size_t len) {
    char resp[64];
    if (_at.sendCmd("AT+CGPADDR=1", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) != ATResult::OK)
        return false;
    // +CGPADDR: 1,"192.168.1.x"
    const char* p = strchr(resp, '"');
    if (!p) return false;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i < len - 1) buf[i++] = *p++;
    buf[i] = '\0';
    return i > 0;
}

bool Network::ping(const char* host, uint32_t timeoutMs) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CIPPING=\"%s\",1,32,10", host);
    return _at.sendCmd(cmd, "+CIPPING:", timeoutMs) == ATResult::OK;
}

bool Network::setPreferredMode(const char* mode) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CNMP=%s", mode);
    return _at.sendCmd(cmd) == ATResult::OK;
}
