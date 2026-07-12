#pragma once
#include "ATClient.h"

// Phase 6 — Voice Calls

enum class CallState {
    IDLE,
    DIALING,
    RINGING,   // incoming
    ACTIVE,
    HELD,
    BUSY,
    NO_ANSWER,
    FAILED
};

struct CallInfo {
    int       index;
    CallState state;
    bool      incoming;
    char      number[24];
};

using CallStateCallback = void (*)(const CallInfo& info, void* ctx);

class Call {
public:
    explicit Call(ATClient& at);

    bool begin();   // register +CLCC / +CRING URCs

    bool      dial(const char* number);
    bool      answer();
    bool      hangup();
    bool      holdActive();
    bool      resumeHeld();

    CallState getCallState();
    bool      getCallInfo(CallInfo& info);

    // DTMF tone (char '0'-'9','*','#','A'-'D')
    bool      sendDTMF(char tone);

    // Volume 0-9
    bool      setSpeakerVolume(int level);
    bool      setMicMute(bool mute);

    void      onStateChange(CallStateCallback cb, void* ctx = nullptr);

private:
    ATClient&        _at;
    CallState         _state = CallState::IDLE;
    CallStateCallback _cb    = nullptr;
    void*             _ctx   = nullptr;

    static void _clccURC(const char* urc, void* ctx);
    static void _cringURC(const char* urc, void* ctx);
};
