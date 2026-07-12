# Wiring

Default pins (override before `#include <A7670E.h>` — see `Config.h`):

| Signal | ESP32 GPIO | A7670E pin | Notes |
|---|---|---|---|
| TX (ESP32 → module RX) | 17 | RX | |
| RX (ESP32 ← module TX) | 16 | TX | |
| PWRKEY | 4 | PWRKEY | Pulled LOW ~1.2s to power on |
| RESET | 5 | RESET | Active LOW hardware reset |
| DTR | not connected (-1) | DTR | Optional; only needed for sleep control |

```cpp
#define A7670E_TX_PIN     17
#define A7670E_RX_PIN     16
#define A7670E_PWRKEY_PIN 4
#define A7670E_RESET_PIN  5
#include <A7670E.h>
```

## Power

The A7670E draws current spikes up to ~2A during TX bursts. Power it from
a dedicated 3.7–4.2V source (LiPo or a regulator rated for the spike), not
directly off the ESP32's 3.3V rail or USB VBUS. Share ground between the
ESP32 and the module's power supply.

## Unverified on this hardware revision

PWRKEY polarity (`UART::powerOn()` drives it LOW then HIGH) matches the
common SIMCom reference design, but some carrier boards invert this or add
their own power-sequencing MCU. If `AT` never gets a response after
`powerOn()`, check your specific board's datasheet before assuming it's a
UART/baud issue.
