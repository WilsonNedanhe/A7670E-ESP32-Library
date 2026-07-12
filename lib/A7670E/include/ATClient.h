#pragma once
#include "UART.h"
#include "Response.h"

// AT Command Engine
// Sends AT commands, waits for expected responses, handles URCs.
//
// URC SAFETY MODEL (fixes reentrancy bug present in v0.1):
// URCs are never invoked from inside sendCmd()/_waitResponse(). While a
// transaction is in flight, matched URC lines are queued. They are only
// dispatched from poll(), which the caller must invoke from their own
// main loop() — i.e. outside of any sendCmd() call. This guarantees a
// URC callback can never re-enter the UART/_respBuf state that an
// in-flight sendCmd() still owns.

class ATClient {
public:
    explicit ATClient(UART& uart);

    // Send cmd, wait for response containing 'expect' (default "OK")
    ATResult sendCmd(const char* cmd,
                     const char* expect   = "OK",
                     uint32_t    timeout  = AT_DEFAULT_TIMEOUT,
                     char*       respBuf  = nullptr,
                     size_t      respSize = 0);

    // Send raw data (for SMS PDU, HTTP body, etc.)
    ATResult sendRaw(const uint8_t* data, size_t len,
                     const char* expect = "OK",
                     uint32_t timeout   = AT_DEFAULT_TIMEOUT);

    // Call from loop(). Reads any waiting URCs, queues matches, then
    // dispatches every queued callback. Never call this from inside
    // sendCmd()/sendRaw() — it already isn't, internally.
    void     poll();

    // Register a URC prefix handler (e.g. "+CMTI", "+CLCC")
    void     registerURC(const char* prefix, URCCallback cb, void* ctx = nullptr);

    // Last error code returned by CME/CMS ERROR
    int      lastErrorCode() const { return _lastErrCode; }

    // Full raw response from last sendCmd
    const char* lastResponse() const { return _respBuf; }

private:
    UART& _uart;
    char    _respBuf[AT_RX_BUF_SIZE];
    int     _lastErrCode = 0;

    struct URCEntry {
        char       prefix[32];
        URCCallback cb;
        void*      ctx;
    };
    static constexpr uint8_t MAX_URC = 16;
    URCEntry _urcs[MAX_URC];
    uint8_t  _urcCount = 0;

    // Queue of matched-but-not-yet-invoked URCs. Sized for short lines
    // ("+CMTI: \"SM\",12"); URC payloads are never as large as a full
    // command response, so this is intentionally much smaller than
    // AT_RX_BUF_SIZE.
    static constexpr uint8_t  MAX_PENDING   = 8;
    static constexpr size_t   PENDING_LINE_LEN = 96;
    struct PendingURC {
        uint8_t urcIndex;
        char    line[PENDING_LINE_LEN];
    };
    PendingURC _pending[MAX_PENDING];
    uint8_t    _pendingCount = 0;

    ATResult _waitResponse(const char* expect, uint32_t timeout);
    // Matches line against registered prefixes and queues it. Never
    // invokes the callback directly — safe to call from _waitResponse().
    bool     _dispatchURC(const char* line);
    // Invokes and clears every queued URC. Only called from poll().
    void     _drainPending();
};
