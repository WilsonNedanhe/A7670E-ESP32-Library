#include "UART.h"

UART::UART(HardwareSerial& serial, int txPin, int rxPin,
                          int pwrKey, int rst)
    : _serial(serial), _txPin(txPin), _rxPin(rxPin),
      _pwrKey(pwrKey), _rst(rst) {}

bool UART::begin(uint32_t baud) {
    if (_pwrKey >= 0) {
        pinMode(_pwrKey, OUTPUT);
        digitalWrite(_pwrKey, HIGH);
    }
    if (_rst >= 0) {
        pinMode(_rst, OUTPUT);
        digitalWrite(_rst, HIGH);
    }
    _serial.begin(baud, SERIAL_8N1, _rxPin, _txPin);
    delay(100);
    return true;
}

void UART::powerOn() {
    if (_pwrKey < 0) return;
    digitalWrite(_pwrKey, LOW);
    delay(1200);
    digitalWrite(_pwrKey, HIGH);
    delay(2000);
}

void UART::powerOff() {
    if (_pwrKey < 0) return;
    digitalWrite(_pwrKey, LOW);
    delay(3000);
    digitalWrite(_pwrKey, HIGH);
}

void UART::hardReset() {
    if (_rst < 0) return;
    digitalWrite(_rst, LOW);
    delay(200);
    digitalWrite(_rst, HIGH);
    delay(3000);
}

size_t UART::write(const char* data) {
    return _serial.print(data);
}

size_t UART::write(const uint8_t* buf, size_t len) {
    return _serial.write(buf, len);
}

int UART::available() {
    return _serial.available();
}

int UART::read() {
    return _serial.read();
}

size_t UART::readLine(char* buf, size_t maxLen, uint32_t timeoutMs) {
    size_t idx   = 0;
    uint32_t t0  = millis();

    while (millis() - t0 < timeoutMs) {
        if (_serial.available()) {
            char c = (char)_serial.read();
            if (c == '\n') break;
            if (c != '\r' && idx < maxLen - 1) {
                buf[idx++] = c;
            }
        } else {
            // Bug fix: this loop used to busy-spin with nothing to yield
            // to the scheduler. On ESP32 that starves the idle task, and
            // the loop-task watchdog (~5s default) resets the board on
            // any wait longer than that — which _waitResponse() in
            // ATClient hits routinely (AT_GPS_TIMEOUT alone is 120s).
            // A 1ms yield costs nothing here since we're already gated
            // by timeoutMs, and keeps the WDT fed.
            delay(1);
        }
    }
    buf[idx] = '\0';
    return idx;
}

void UART::flush() {
    _serial.flush();
    while (_serial.available()) _serial.read();
}
