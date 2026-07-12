// Phase 7 — GNSS Fix
// Powers on GPS, waits for a fix, then prints lat/lon every 5 s.

#include <A7670E.h>

A7670E_UART uart;
A7670E_AT   at(uart);
A7670E_GNSS gnss(at);

void setup() {
    Serial.begin(115200);
    delay(1000);

    uart.begin();
    uart.powerOn();
    delay(5000);
    at.sendCmd("ATE0");

    gnss.begin();
    Serial.println("Powering on GNSS...");
    gnss.powerOn();

    Serial.println("Waiting for fix (up to 2 min)...");
    GNSSFix fix;
    if (gnss.waitForFix(fix, AT_GPS_TIMEOUT)) {
        Serial.println("Fix acquired!");
        Serial.printf("  Lat : %.6f\n", fix.latitude);
        Serial.printf("  Lon : %.6f\n", fix.longitude);
        Serial.printf("  Alt : %.1f m\n", fix.altitude);
        Serial.printf("  Spd : %.1f km/h\n", fix.speed);
        Serial.printf("  HDOP: %.1f\n", fix.hdop);
        Serial.printf("  Sats: %d\n", fix.satellites);
        Serial.printf("  Time: %s\n", fix.utcTime);
    } else {
        Serial.println("No fix within timeout");
    }
}

void loop() {
    // Poll for fresh fixes every 5 s
    static uint32_t t0 = millis();
    if (millis() - t0 > 5000) {
        t0 = millis();
        GNSSFix fix;
        if (gnss.getFix(fix) && fix.valid) {
            Serial.printf("[GPS] %.6f, %.6f  alt=%.0fm  sats=%d  %s\n",
                          fix.latitude, fix.longitude,
                          fix.altitude, fix.satellites, fix.utcTime);
        }
    }
}
