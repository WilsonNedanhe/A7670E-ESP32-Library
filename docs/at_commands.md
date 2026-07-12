# AT command reference (by module)

Non-exhaustive — the commands each module actually issues today. Cross-
check against the A7670E AT Command Manual for full parameter sets.

| Module | Commands used |
|---|---|
| `UART` (power) | PWRKEY/RESET GPIO sequencing (no AT commands) |
| `ATClient` | Generic `sendCmd()`/`sendRaw()` — no fixed command set |
| `Network` | `AT+CGDCONT`, `AT+CGATT`, `AT+CREG`/`AT+CEREG`, `AT+CSQ`, `AT+COPS`, `AT+CGPADDR` |
| `SMS` | `AT+CMGF`, `AT+CPMS`, `AT+CMGS`, `AT+CMGR`, `AT+CMGL`, `AT+CMGD`, `AT+CMGDA`, `+CMTI:` URC |
| `Call` | `ATD`, `ATA`, `ATH`, `+CLCC:` / `+CRING:` URCs |
| `GPS` | `AT+CGNSSPWR`, `AT+CGNSSINFO` |
| `HTTP` | `AT+HTTPINIT`, `AT+HTTPPARA`, `AT+HTTPACTION`, `AT+HTTPREAD`, `AT+HTTPTERM` |
| `MQTT` | `AT+CMQTTSTART`, `AT+CMQTTCONNECT`, `AT+CMQTTPUB`, `AT+CMQTTSUB`, `AT+CMQTTDISC` |
| `DeviceInfo` | `AT+CGMI`, `AT+CGMM`, `AT+CGMR`, `AT+CGSN`, `AT+CCID` |

## Timeouts

Defined in `Config.h`: `AT_DEFAULT_TIMEOUT` (3s), `AT_LONG_TIMEOUT` (30s),
`AT_NETWORK_TIMEOUT` (60s), `AT_GPS_TIMEOUT` (120s), `AT_HTTP_TIMEOUT`
(60s). If you add a command that can legitimately take longer than these
(e.g. a cold GPS fix in poor sky view), don't loosen the global constant —
pass an explicit `timeout` argument to that one `sendCmd()` call.
