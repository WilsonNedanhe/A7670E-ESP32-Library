// Phase 4 — Network Registration & LTE Data
// Connects to LTE, prints IP, signal info, and pings Google DNS.

#include <A7670E.h>

UART    uart;
ATClient      at(uart);
Network net(at);

void setup() {
    Serial.begin(115200);
    delay(1000);

    uart.begin();
    uart.powerOn();
    delay(5000);
    at.sendCmd("ATE0");

    // ── Change APN to match your SIM carrier ──
    net.setAPN("internet");

    Serial.println("Waiting for network registration...");
    uint32_t t0 = millis();
    while (millis() - t0 < 60000) {
        RegStatus s = net.getRegistrationStatus();
        if (s == RegStatus::REGISTERED_HOME || s == RegStatus::REGISTERED_ROAMING) break;
        Serial.print(".");
        delay(2000);
    }
    Serial.println();

    if (!net.activate(30000)) {
        Serial.println("PDP activation failed"); return;
    }

    char ip[20];
    net.getLocalIP(ip, sizeof(ip));
    Serial.printf("Local IP: %s\n", ip);

    SignalInfo sig;
    net.getSignalInfo(sig);
    Serial.printf("Operator: %s  RSSI: %d dBm  CSQ: %d\n",
                  sig.operatorName, sig.rssi, sig.csq);

    Serial.print("Pinging 8.8.8.8 ... ");
    Serial.println(net.ping("8.8.8.8") ? "PASS" : "FAIL");
}

void loop() {
    at.poll();
}
