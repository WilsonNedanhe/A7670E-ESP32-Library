#include "SMS.h"
#include <string.h>
#include <stdio.h>

SMS::SMS(ATClient& at) : _at(at) {}

bool SMS::begin() {
    // Text mode
    if (_at.sendCmd("AT+CMGF=1") != ATResult::OK) return false;
    // Store to SIM
    _at.sendCmd("AT+CPMS=\"SM\",\"SM\",\"SM\"");
    // URC for new SMS
    _at.registerURC("+CMTI:", _urcHandler, this);
    return true;
}

bool SMS::sendSMS(const char* number, const char* text) {
    char cmd[40];
    snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", number);

    // Wait for '>' prompt
    if (_at.sendCmd(cmd, ">", AT_DEFAULT_TIMEOUT) != ATResult::OK) return false;

    // Send text followed by Ctrl-Z (0x1A).
    // body must hold SMS_MAX_LEN chars + terminator (0x1A) + '\0'.
    // Bug fix: the old code let snprintf truncate to sizeof(body)-1
    // (=SMS_MAX_LEN+1 chars) and then wrote two more bytes past that,
    // a 1-byte stack overflow whenever text was longer than SMS_MAX_LEN.
    // Clamp the copy length explicitly instead of trusting snprintf's
    // truncation point to leave room for what we append after it.
    char body[SMS_MAX_LEN + 2];
    size_t len = strlen(text);
    if (len > SMS_MAX_LEN) len = SMS_MAX_LEN;
    memcpy(body, text, len);
    body[len]   = 0x1A;
    body[len+1] = '\0';
    return _at.sendRaw((const uint8_t*)body, len + 1, "+CMGS:", AT_LONG_TIMEOUT) == ATResult::OK;
}

bool SMS::readSMS(int index, SMSMessage& msg) {
    char cmd[20];
    snprintf(cmd, sizeof(cmd), "AT+CMGR=%d", index);
    char resp[512];
    if (_at.sendCmd(cmd, "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp)) != ATResult::OK)
        return false;
    msg.index = index;
    return _parseSMS(resp, msg);
}

bool SMS::_parseSMS(const char* raw, SMSMessage& msg) {
    // +CMGR: "REC UNREAD","+1234567890",,"YY/MM/DD,HH:MM:SS+ZZ"
    const char* p = strstr(raw, "+CMGR:");
    if (!p) { p = strstr(raw, "+CMGL:"); }
    if (!p) return false;

    // Extract status
    const char* q = strchr(p, '"');
    if (!q) return false; q++;
    size_t i = 0;
    while (*q && *q != '"' && i < sizeof(msg.status) - 1) msg.status[i++] = *q++;
    msg.status[i] = '\0'; q++;

    // Extract sender
    q = strchr(q, '"');
    if (!q) return false; q++;
    i = 0;
    while (*q && *q != '"' && i < sizeof(msg.sender) - 1) msg.sender[i++] = *q++;
    msg.sender[i] = '\0';

    // Text is on the next line
    q = strchr(q, '\n');
    if (!q) return false; q++;
    i = 0;
    while (*q && *q != '\r' && *q != '\n' && i < SMS_MAX_LEN) msg.text[i++] = *q++;
    msg.text[i] = '\0';
    return true;
}

bool SMS::deleteAllSMS() {
    return _at.sendCmd("AT+CMGDA=\"DEL ALL\"", "OK", AT_LONG_TIMEOUT) == ATResult::OK;
}

bool SMS::deleteSMS(int index) {
    char cmd[20];
    snprintf(cmd, sizeof(cmd), "AT+CMGD=%d", index);
    return _at.sendCmd(cmd) == ATResult::OK;
}

int SMS::listUnread(SMSMessage* msgs, int maxCount) {
    char resp[2048];
    if (_at.sendCmd("AT+CMGL=\"REC UNREAD\"", "OK", AT_LONG_TIMEOUT,
                    resp, sizeof(resp)) != ATResult::OK) return 0;
    int count = 0;
    char* p = resp;
    while ((p = strstr(p, "+CMGL:")) && count < maxCount) {
        msgs[count].index = atoi(p + 6);
        _parseSMS(p, msgs[count]);
        count++;
        p++;
    }
    return count;
}

int SMS::listAll(SMSMessage* msgs, int maxCount) {
    char resp[2048];
    if (_at.sendCmd("AT+CMGL=\"ALL\"", "OK", AT_LONG_TIMEOUT,
                    resp, sizeof(resp)) != ATResult::OK) return 0;
    int count = 0;
    char* p = resp;
    while ((p = strstr(p, "+CMGL:")) && count < maxCount) {
        msgs[count].index = atoi(p + 6);
        _parseSMS(p, msgs[count]);
        count++;
        p++;
    }
    return count;
}

void SMS::onReceived(SMSReceivedCallback cb, void* ctx) {
    _cb  = cb;
    _ctx = ctx;
}

void SMS::_urcHandler(const char* urc, void* ctx) {
    SMS* self = static_cast<SMS*>(ctx);
    if (!self->_cb) return;
    // +CMTI: "SM",<index>
    const char* p = strrchr(urc, ',');
    if (!p) return;
    int index = atoi(p + 1);
    SMSMessage msg;
    if (self->readSMS(index, msg)) {
        self->_cb(msg, self->_ctx);
    }
}
