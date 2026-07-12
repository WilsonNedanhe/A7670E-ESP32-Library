// Phase 3 — Device Information
// Prints IMEI, IMSI, ICCID, firmware, model to Serial.

#include <A7670E.h>

A7670E_UART       uart;
A7670E_AT         at(uart);
A7670E_DeviceInfo info(at);

void setup() {
    Serial.begin(115200);
    delay(1000);

    uart.begin();
    uart.powerOn();
    delay(5000);
    at.sendCmd("ATE0");

    SIMState sim = info.getSIMState();
    if (sim == SIMState::PIN_REQUIRED) {
        Serial.println("SIM locked — unlocking with PIN 1234");
        info.unlockSIM("1234");
        delay(3000);
    }

    DeviceInfo d;
    if (info.getDeviceInfo(d)) {
        Serial.printf("Model       : %s\n", d.model);
        Serial.printf("Manufacturer: %s\n", d.manufacturer);
        Serial.printf("Firmware    : %s\n", d.firmware);
        Serial.printf("IMEI        : %s\n", d.imei);
        Serial.printf("IMSI        : %s\n", d.imsi);
        Serial.printf("ICCID       : %s\n", d.iccid);
    } else {
        Serial.println("Failed to read device info");
    }
}

void loop() {}
