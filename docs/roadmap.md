# Roadmap

## Done (v0.2.0)
- Restructured into a standard PlatformIO library layout (`lib/A7670E`)
- Fixed: `UART::readLine()` busy-wait causing ESP32 watchdog resets on
  long timeouts (GPS fix waits in particular)
- Fixed: 1-byte stack buffer overflow in `SMS::sendSMS()` for messages at
  the 160-char boundary
- Fixed: URC callback reentrancy — callbacks can no longer fire mid-
  transaction and corrupt `_respBuf`
- Added a native (`pio test -e native`) regression test for the SMS
  buffer fix

## Next
- [ ] Wire up `test_uart`/`test_network`/`test_sms` against real hardware
      (currently `TEST_IGNORE`d placeholders — see `test/`)
- [ ] PDU-mode SMS (current implementation is text-mode only; PDU is
      needed for non-GSM7 alphabets / concatenated SMS)
- [ ] TLS config surface for `HTTP`/`MQTT` (currently assumes plaintext or
      module-default cert handling)
- [ ] DTR-based sleep/wake support
- [ ] Move `_pending` URC queue size (`MAX_PENDING = 8`) to a `Config.h`
      constant so high-URC-volume applications can tune it without editing
      library internals

## Explicitly out of scope for this library
- PPP/network-interface integration (this library treats the module as an
  AT-command peripheral, not a network interface for the ESP32's TCP/IP
  stack)
