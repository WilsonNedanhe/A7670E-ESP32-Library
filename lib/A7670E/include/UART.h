#pragma once
#include "Config.h"

// Phase 1 — UART Driver
// Handles low-level serial I/O, power key toggling, and hardware reset.

class UART {
public:
    UART(HardwareSerial& serial = A7670E_SERIAL,
                int txPin  = A7670E_TX_PIN,
                int rxPin  = A7670E_RX_PIN,
                int pwrKey = A7670E_PWRKEY_PIN,
                int rst    = A7670E_RESET_PIN);

    bool    begin(uint32_t baud = A7670E_BAUD);
    void    powerOn();
    void    powerOff();
    void    hardReset();

    size_t  write(const char* data);
    size_t  write(const uint8_t* buf, size_t len);
    int     available();
    int     read();
    size_t  readLine(char* buf, size_t maxLen, uint32_t timeoutMs = AT_DEFAULT_TIMEOUT);
    void    flush();

    HardwareSerial& serial() { return _serial; }

private:
    HardwareSerial& _serial;
    int _txPin, _rxPin, _pwrKey, _rst;
};
