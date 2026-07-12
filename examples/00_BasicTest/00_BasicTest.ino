// Phase 0 — Basic AT echo test
// Verifies UART wiring and that the module responds to AT commands.

#include <A7670E.h>

A7670E_UART  uart;
A7670E_AT    at(uart);

static bool waitForAT(uint32_t timeoutMs) {
    uint32_t t0 = millis();

    while (millis() - t0 < timeoutMs) {
        ATResult r = at.sendCmd("AT");
        if (r == ATResult::OK) {
            return true;
        }

        Serial.printf("AT failed: %d\n", static_cast<int>(r));
        delay(1000);
    }

    return false;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== A7670E Basic Test ===");

    uart.begin();
    uart.powerOn();
    delay(5000);  // allow the module to finish booting

    Serial.println("Sending AT...");
    if (waitForAT(30000)) {
        Serial.println("Result: OK");
    } else {
        Serial.println("Result: FAIL");
        Serial.print("Last response: ");
        Serial.println(at.lastResponse());
        return;
    }

    Serial.println("Sending ATE0 (echo off)...");
    if (at.sendCmd("ATE0") != ATResult::OK) {
        Serial.print("ATE0 failed, response: ");
        Serial.println(at.lastResponse());
        return;
    }

    Serial.println("Done.");
}

void loop() {
    at.poll();  // handle any URCs
}
