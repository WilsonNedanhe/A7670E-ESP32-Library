# A7670E-ESP32-Library

Arduino/PlatformIO library for driving the **SIMCom A7670E** LTE Cat-1 module
from an ESP32 over hardware UART.

## Modules

| Header | Purpose |
|---|---|
| `UART.h` | Serial I/O, PWRKEY/RESET sequencing |
| `ATClient.h` / `Response.h` | AT command engine, timeouts, URC dispatch |
| `Network.h` | Registration state, signal quality, operator info |
| `SMS.h` | Send/read/list/delete SMS, incoming-SMS URC |
| `Call.h` | Voice call control and state URCs |
| `GPS.h` | GNSS fix acquisition |
| `HTTP.h` | HTTP/HTTPS client |
| `MQTT.h` | MQTT publish/subscribe |
| `DeviceInfo.h` | IMEI, ICCID, firmware version |
| `Logger.h` | SD-card logging |
| `TelecomAnalyzer.h` | KPI/diagnostics helpers |
| `MLDataGen.h` | Structured data export for ML pipelines |

## Install (PlatformIO)

```ini
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
    at.poll();   // must run every loop() — see docs/architecture.md
}
```

## Important: URC / `poll()` contract

`ATClient::poll()` must be called from your main `loop()`, and never from
inside a `sendCmd()`/`sendRaw()` call. URC callbacks are only ever invoked
from `poll()`. Don't call `sendCmd()` from inside a URC callback that fires
from `_waitResponse()` — it can't, by construction — but also don't call it
from a `poll()`-fired callback while another transaction might still be
pending; queue the work and issue it on the next `loop()` pass instead. Full
rationale in [`docs/architecture.md`](../../docs/architecture.md).

See `examples/` for full sketches and `docs/wiring.md` for pinout.

## License

MIT — see `LICENSE`.
