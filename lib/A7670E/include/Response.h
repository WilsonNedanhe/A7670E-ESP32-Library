#pragma once
#include <stdint.h>
#include <stddef.h>

// Result / URC types shared by ATClient and every module built on top of it.

enum class ATResult {
    OK,
    ERROR,
    CME_ERROR,
    CMS_ERROR,
    TIMEOUT,
    NO_MATCH
};

// Unsolicited Result Code callback.
// IMPORTANT: this fires from ATClient::poll(), which must only be called
// from your main loop() — never from inside a sendCmd()/sendRaw() call.
// Do NOT call sendCmd()/sendRaw() from inside a URC callback; if you need
// to react with another AT command, set a flag here and issue the command
// from loop() on the next iteration. See docs/architecture.md.
using URCCallback = void (*)(const char* urc, void* ctx);
