# A7670E-ESP32-Library

Arduino/PlatformIO library for driving the **SIMCom A7670E** LTE Cat-1
module from an ESP32 over hardware UART.

```
A7670E-ESP32-Library/
├── lib/A7670E/          the actual library (this is what gets installed)
├── examples/            one sketch per module, numbered 01-11
├── src/main.cpp          demo entry point (proves lib links on esp32dev)
├── test/                 test_at runs natively in CI; the rest are
│                          hardware-in-the-loop placeholders — see below
├── docs/                 architecture, wiring, AT command reference, roadmap
└── platformio.ini
```

## Install

```ini
; platformio.ini
lib_deps =
    https://github.com/WilsonNedanhe/A7670E-ESP32-Library.git
```

## Quick start

```cpp
#include <A7670E.h>

UART      uart;
ATClient  at(uart);

void setup() {
    uart.begin();
    uart.powerOn();
    at.sendCmd("AT");
}

void loop() {
    at.poll();   // required — see docs/architecture.md before skipping this
}
```

**Read `docs/architecture.md` before writing a new module or a URC
callback.** The one rule that matters: URC callbacks only ever fire from
`poll()`, called from `loop()`. Never call `sendCmd()`/`sendRaw()` from
anywhere else.

## Status

v0.2.0. See `CHANGELOG.md` for what changed from v0.1, including three
fixed bugs (watchdog resets, an SMS buffer overflow, URC reentrancy) that
were real, reproducible defects — not style cleanup.

Hardware-in-the-loop tests (`test/test_uart`, `test/test_network`,
`test/test_sms`) are scaffolded but not yet implemented — they currently
`TEST_IGNORE`. `test/test_at` runs for real in CI against a faked serial
and is the one test suite you can currently trust.

## License

MIT — see `LICENSE`.
