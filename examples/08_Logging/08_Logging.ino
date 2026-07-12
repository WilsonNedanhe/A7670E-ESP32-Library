// Phase 10 — SD Card Logging
// Writes timestamped log entries to /a7670e.log on an SD card.

#include <A7670E.h>

// SD chip-select pin
#define SD_CS_PIN 5

A7670E_UART   uart;
A7670E_AT     at(uart);
A7670E_Logger logger(SD_CS_PIN);

void setup() {
    Serial.begin(115200);
    delay(1000);

    uart.begin();
    uart.powerOn();
    delay(5000);

    if (!logger.begin()) {
        Serial.println("SD logger init failed — logging to Serial only");
    }

    logger.setTimestamp("2025-01-01 00:00:00");

    logger.log(LogLevel::INFO,  "BOOT",  "System started");
    logger.log(LogLevel::DEBUG, "UART",  "Module power-on pulse sent");
    logger.logf(LogLevel::INFO, "HEAP",  "Free heap: %u bytes", (unsigned)ESP.getFreeHeap());
    logger.logKV(LogLevel::INFO, "VER",  "firmware", "1.0.0");

    // JSON log
    logger.logJSON(LogLevel::INFO, "NET",
                   "{\"status\":\"connecting\",\"apn\":\"internet\"}");

    logger.flush();
    Serial.println("Log written. Check SD card /a7670e.log");
}

void loop() {
    // Log a heartbeat every 30 s
    static uint32_t t0 = millis();
    if (millis() - t0 > 30000) {
        t0 = millis();
        logger.logf(LogLevel::INFO, "HB",
                    "uptime=%lus heap=%u",
                    millis() / 1000, (unsigned)ESP.getFreeHeap());
        logger.flush();
    }
}
