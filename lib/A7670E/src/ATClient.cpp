#include "ATClient.h"
#include <string.h>
#include <stdio.h>

ATClient::ATClient(UART& uart) : _uart(uart) {
    memset(_respBuf, 0, sizeof(_respBuf));
    memset(_urcs, 0, sizeof(_urcs));
}

ATResult ATClient::sendCmd(const char* cmd, const char* expect,
                             uint32_t timeout, char* respBuf, size_t respSize) {
    _uart.flush();
    DBG(">> %s\n", cmd);

    _uart.write(cmd);
    _uart.write("\r\n");

    ATResult result = _waitResponse(expect, timeout);

    if (respBuf && respSize > 0) {
        strncpy(respBuf, _respBuf, respSize - 1);
        respBuf[respSize - 1] = '\0';
    }
    return result;
}

ATResult ATClient::sendRaw(const uint8_t* data, size_t len,
                             const char* expect, uint32_t timeout) {
    _uart.write(data, len);
    return _waitResponse(expect, timeout);
}

ATResult ATClient::_waitResponse(const char* expect, uint32_t timeout) {
    size_t   idx = 0;
    uint32_t t0  = millis();

    memset(_respBuf, 0, sizeof(_respBuf));

    while (millis() - t0 < timeout) {
        char lineBuf[AT_RX_BUF_SIZE];
        size_t len = _uart.readLine(lineBuf, sizeof(lineBuf), 200);

        if (len == 0) continue;

        DBG("<< %s\n", lineBuf);

        // Accumulate into _respBuf
        if (idx + len + 2 < AT_RX_BUF_SIZE) {
            memcpy(_respBuf + idx, lineBuf, len);
            idx += len;
            _respBuf[idx++] = '\n';
        }

        // Check for terminal responses
        if (strcmp(lineBuf, "OK") == 0) {
            return (expect == nullptr || strstr(_respBuf, expect))
                   ? ATResult::OK : ATResult::NO_MATCH;
        }
        if (strcmp(lineBuf, "ERROR") == 0) {
            return ATResult::ERROR;
        }
        if (strncmp(lineBuf, "+CME ERROR:", 11) == 0) {
            _lastErrCode = atoi(lineBuf + 11);
            return ATResult::CME_ERROR;
        }
        if (strncmp(lineBuf, "+CMS ERROR:", 11) == 0) {
            _lastErrCode = atoi(lineBuf + 11);
            return ATResult::CMS_ERROR;
        }

        // Check custom expect string (non-OK terminal)
        if (expect && strstr(lineBuf, expect)) {
            return ATResult::OK;
        }

        // Might be a URC arriving mid-transaction (e.g. +CMTI while we're
        // waiting on an unrelated command). Queue it — do NOT invoke the
        // callback here. Invoking it here would let user code call
        // sendCmd() again while this _waitResponse() call still owns the
        // UART and _respBuf, corrupting both. It gets dispatched later
        // from poll(), once this transaction has returned.
        _dispatchURC(lineBuf);
    }
    return ATResult::TIMEOUT;
}

bool ATClient::_dispatchURC(const char* line) {
    for (uint8_t i = 0; i < _urcCount; i++) {
        if (strncmp(line, _urcs[i].prefix, strlen(_urcs[i].prefix)) == 0) {
            if (_pendingCount < MAX_PENDING) {
                PendingURC& p = _pending[_pendingCount++];
                p.urcIndex = i;
                strncpy(p.line, line, PENDING_LINE_LEN - 1);
                p.line[PENDING_LINE_LEN - 1] = '\0';
            }
            // If the queue is full we drop the URC rather than overflow;
            // increase MAX_PENDING if your application bursts more than
            // 8 unsolicited events between poll() calls.
            return true;
        }
    }
    return false;
}

void ATClient::_drainPending() {
    // Snapshot count first: a callback may itself trigger registerURC()
    // or otherwise touch state, but it must never grow the same queue
    // it's currently being drained from.
    uint8_t count = _pendingCount;
    _pendingCount = 0;
    for (uint8_t i = 0; i < count; i++) {
        URCEntry& urc = _urcs[_pending[i].urcIndex];
        if (urc.cb) urc.cb(_pending[i].line, urc.ctx);
    }
}

void ATClient::poll() {
    char lineBuf[AT_RX_BUF_SIZE];
    while (_uart.available()) {
        size_t len = _uart.readLine(lineBuf, sizeof(lineBuf), 50);
        if (len > 0) {
            DBG("URC: %s\n", lineBuf);
            _dispatchURC(lineBuf);
        }
    }
    // Safe to invoke callbacks here: poll() is only ever called from the
    // user's loop(), never from inside sendCmd()/sendRaw().
    _drainPending();
}

void ATClient::registerURC(const char* prefix, URCCallback cb, void* ctx) {
    if (_urcCount >= MAX_URC) return;
    strncpy(_urcs[_urcCount].prefix, prefix, sizeof(_urcs[0].prefix) - 1);
    _urcs[_urcCount].cb  = cb;
    _urcs[_urcCount].ctx = ctx;
    _urcCount++;
}
