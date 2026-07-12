#include "Call.h"
#include <string.h>
#include <stdio.h>

Call::Call(ATClient& at) : _at(at) {}

bool Call::begin() {
    _at.registerURC("+CLCC", _clccURC,  this);
    _at.registerURC("+CRING", _cringURC, this);
    // Enable call state URCs
    _at.sendCmd("AT+CLCC=1");
    return true;
}

bool Call::dial(const char* number) {
    char cmd[36];
    snprintf(cmd, sizeof(cmd), "ATD%s;", number);
    bool ok = (_at.sendCmd(cmd, "OK", AT_DEFAULT_TIMEOUT) == ATResult::OK);
    if (ok) _state = CallState::DIALING;
    return ok;
}

bool Call::answer() {
    bool ok = (_at.sendCmd("ATA") == ATResult::OK);
    if (ok) _state = CallState::ACTIVE;
    return ok;
}

bool Call::hangup() {
    bool ok = (_at.sendCmd("ATH") == ATResult::OK);
    if (ok) _state = CallState::IDLE;
    return ok;
}

bool Call::holdActive() {
    return _at.sendCmd("AT+CHLD=2") == ATResult::OK;
}

bool Call::resumeHeld() {
    return _at.sendCmd("AT+CHLD=2") == ATResult::OK;
}

CallState Call::getCallState() {
    return _state;
}

bool Call::getCallInfo(CallInfo& info) {
    char resp[128];
    if (_at.sendCmd("AT+CLCC", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) != ATResult::OK)
        return false;

    const char* p = strstr(resp, "+CLCC:");
    if (!p) { info.state = CallState::IDLE; return true; }

    // +CLCC: <idx>,<dir>,<stat>,<mode>,<mpty>,"<number>",<type>
    int idx, dir, stat, mode, mpty;
    char num[24];
    if (sscanf(p, "+CLCC: %d,%d,%d,%d,%d,\"%23[^\"]\"",
               &idx, &dir, &stat, &mode, &mpty, num) >= 5) {
        info.index    = idx;
        info.incoming = (dir == 1);
        strncpy(info.number, num, sizeof(info.number) - 1);
        switch (stat) {
            case 0: info.state = CallState::ACTIVE;  break;
            case 1: info.state = CallState::HELD;    break;
            case 2: info.state = CallState::DIALING; break;
            case 4: info.state = CallState::RINGING; break;
            default: info.state = CallState::IDLE;
        }
    }
    return true;
}

bool Call::sendDTMF(char tone) {
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "AT+VTS=%c", tone);
    return _at.sendCmd(cmd) == ATResult::OK;
}

bool Call::setSpeakerVolume(int level) {
    char cmd[20];
    snprintf(cmd, sizeof(cmd), "AT+CLVL=%d", level);
    return _at.sendCmd(cmd) == ATResult::OK;
}

bool Call::setMicMute(bool mute) {
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "AT+CMUT=%d", mute ? 1 : 0);
    return _at.sendCmd(cmd) == ATResult::OK;
}

void Call::onStateChange(CallStateCallback cb, void* ctx) {
    _cb  = cb;
    _ctx = ctx;
}

void Call::_clccURC(const char* urc, void* ctx) {
    Call* self = static_cast<Call*>(ctx);
    CallInfo info;
    self->getCallInfo(info);
    self->_state = info.state;
    if (self->_cb) self->_cb(info, self->_ctx);
}

void Call::_cringURC(const char* urc, void* ctx) {
    Call* self = static_cast<Call*>(ctx);
    self->_state = CallState::RINGING;
    CallInfo info;
    info.state    = CallState::RINGING;
    info.incoming = true;
    info.index    = 1;
    info.number[0] = '\0';
    if (self->_cb) self->_cb(info, self->_ctx);
}
