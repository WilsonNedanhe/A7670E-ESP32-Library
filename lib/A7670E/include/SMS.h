#pragma once
#include "ATClient.h"

// Phase 5 — SMS (text + PDU mode)

struct SMSMessage {
    int    index;
    char   status[16];   // "REC UNREAD", "REC READ", "STO UNSENT", "STO SENT"
    char   sender[24];
    char   timestamp[24];
    char   text[SMS_MAX_LEN + 1];
};

using SMSReceivedCallback = void (*)(const SMSMessage& msg, void* ctx);

class SMS {
public:
    explicit SMS(ATClient& at);

    bool begin();  // switch to text mode, register URC

    bool sendSMS(const char* number, const char* text);
    bool readSMS(int index, SMSMessage& msg);
    bool deleteAllSMS();
    bool deleteSMS(int index);

    // List unread messages (fills array, returns count)
    int  listUnread(SMSMessage* msgs, int maxCount);
    int  listAll   (SMSMessage* msgs, int maxCount);

    // Called from ATClient poll loop when +CMTI arrives
    void onReceived(SMSReceivedCallback cb, void* ctx = nullptr);

private:
    ATClient&         _at;
    SMSReceivedCallback _cb  = nullptr;
    void*              _ctx = nullptr;

    static void _urcHandler(const char* urc, void* ctx);
    bool _parseSMS(const char* raw, SMSMessage& msg);
};
