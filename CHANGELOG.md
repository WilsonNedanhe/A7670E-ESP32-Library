# Changelog

## [0.2.0] — Restructure + bug fixes

### Fixed
- **Watchdog resets on ESP32** — `UART::readLine()` busy-waited with no
  yield, starving the idle task. Any wait near/over ~5s (routine at
  `AT_GPS_TIMEOUT`'s 120s) reset the board. Now yields every idle
  iteration.
- **1-byte stack buffer overflow in `SMS::sendSMS()`** — messages at
  exactly the 160-char boundary wrote one byte past `body[]`. Length is
  now clamped explicitly before the terminator is written, instead of
  relying on `snprintf`'s truncation point.
- **URC callback reentrancy** — a URC arriving mid-transaction (e.g.
  `+CMTI:` while waiting on an unrelated command) used to invoke its
  callback immediately, which could call `sendCmd()` again and corrupt
  the in-flight transaction's `_respBuf`. URCs are now queued during
  transactions and only dispatched from `ATClient::poll()`, called from
  `loop()` — see `docs/architecture.md`.

### Changed
- Repository restructured to a standard PlatformIO library layout:
  library code moved to `lib/A7670E/{include,src}`, examples renumbered
  and consolidated to match module names, `ATClient`/`Response` split
  out of the old monolithic AT header.
- Class/file names dropped the `A7670E_` prefix (`A7670E_UART` → `UART`,
  `A7670E_SMS` → `SMS`, etc). `A7670E_*` pin/timeout macros in `Config.h`
  are unchanged — that's the public configuration surface and renaming
  it would break every existing sketch for no benefit.
- `A7670E_Voice` renamed to `Call` (file and class) to match the
  `examples/07_Call` naming.

### Added
- `Version.h` (`A7670E_LIB_VERSION`).
- Native (`pio test -e native`) regression test for the `sendSMS`
  overflow fix, run against the real library code via a fake serial —
  not hardware-in-the-loop.
- CI (`.github/workflows/build.yml`): builds the library + every example
  for `esp32dev`, runs native tests.
- `docs/architecture.md`, `docs/wiring.md`, `docs/at_commands.md`,
  `docs/roadmap.md`.

### Not changed (scope note)
`DeviceInfo`, `Logger`, `TelecomAnalyzer`, and `MLDataGen` weren't in the
requested skeleton's header list but were working modules in v0.1 — kept
them rather than deleting real functionality; they live in
`lib/A7670E/include|src` alongside the others and have their own examples
(`08`–`11`).

## [0.1.0] — Initial release
Voice, SMS, GNSS, HTTP, MQTT, SD logging, network diagnostics, ML data
collection modules over ESP32 hardware UART.
