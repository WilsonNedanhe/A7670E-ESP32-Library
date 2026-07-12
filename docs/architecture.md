# Architecture

## Layers

```
Your sketch / app code
        │
   Network / SMS / Call / GPS / HTTP / MQTT / DeviceInfo / Logger / ...
        │              (each module wraps a set of AT commands)
   ATClient  ── owns _respBuf, transaction state, URC registry
        │
   UART      ── PWRKEY/RESET sequencing, raw serial read/write
        │
   HardwareSerial (Serial2 by default)
```

Every module (`SMS`, `Network`, `HTTP`, ...) takes a reference to a shared
`ATClient` in its constructor. There's exactly one `ATClient` per modem —
don't construct two against the same `UART`.

## The URC / `poll()` contract

This is the part that's easy to get wrong, and got it wrong in v0.1.

`ATClient::sendCmd()` blocks (up to its timeout) waiting for a terminal
response (`OK`, `ERROR`, `+CME ERROR`, ...). While it's waiting, the modem
can interleave unsolicited lines — an incoming SMS (`+CMTI:`), a call state
change (`+CLCC:`), a network URC — with the response it's actually waiting
for.

**Rule: URC callbacks are only ever invoked from `ATClient::poll()`, and
`poll()` must only be called from your `loop()` — never from inside a
`sendCmd()`/`sendRaw()` call.**

Concretely:
- If a URC line shows up *while* `_waitResponse()` is blocked inside some
  other `sendCmd()`, it gets queued (`_dispatchURC`), not invoked.
- `poll()`, called from your `loop()`, drains that queue and invokes the
  registered callbacks — at a point where no transaction is in flight, so
  it's safe for those callbacks to themselves call `sendCmd()` (e.g.
  `SMS`'s `+CMTI:` handler calling `readSMS()`).

**What not to do:** don't call `sendCmd()`/`sendRaw()` directly from code
that isn't either (a) your `loop()`, or (b) a URC callback invoked from
`poll()`. Two overlapping transactions on one `ATClient` will corrupt
`_respBuf` and desync response parsing — there's no mutex here, this is a
single-threaded cooperative design, same as the rest of Arduino.

## Buffers are fixed-size, on purpose

`AT_RX_BUF_SIZE`, `SMS_MAX_LEN`, `HTTP_RESP_BUF_SIZE` (see `Config.h`) are
all compile-time constants. Nothing allocates. When you write a new module,
follow the existing pattern in `SMS::sendSMS()`: compute the length you're
about to write, clamp it explicitly against the destination buffer size,
*then* write — don't rely on `snprintf`'s truncation behavior to leave you
room for anything you append afterward. That's exactly the bug that shipped
in v0.1 (see `CHANGELOG.md`).

## Why watchdog resets matter here

`UART::readLine()` blocks up to its timeout waiting for bytes. Some of
those timeouts are long — `AT_GPS_TIMEOUT` is 120s. On ESP32, the default
loop-task watchdog is ~5s. Any blocking loop with no `yield()`/`delay()` in
its idle branch will reset the board mid-wait. `readLine()` now yields
every idle iteration; if you write new blocking loops anywhere in this
library, do the same.
