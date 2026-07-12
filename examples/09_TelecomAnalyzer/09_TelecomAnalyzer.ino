// Phase 11 — Telecom Analyzer
// Continuously samples signal metrics, scans neighbours, runs diagnostics.

#include <A7670E.h>

#define SD_CS_PIN 5

A7670E_UART           uart;
A7670E_AT             at(uart);
A7670E_Network        net(at);
A7670E_Logger         logger(SD_CS_PIN);
A7670E_TelecomAnalyzer analyzer(at, net, &logger);

void setup() {
    Serial.begin(115200);
    delay(1000);

    uart.begin();
    uart.powerOn();
    delay(5000);
    at.sendCmd("ATE0");

    logger.begin();

    net.setAPN("internet");
    net.activate(30000);

    // Full diagnostic snapshot
    DiagReport report;
    if (analyzer.runDiagnostics(report)) {
        Serial.printf("Serving cell  : %d  RSSI: %d dBm\n",
                      report.serving.cellId, report.serving.rssi);
        Serial.printf("Neighbours    : %d\n", report.neighbourCount);
        Serial.printf("Data connected: %s\n", report.dataConnected ? "YES" : "NO");
        Serial.printf("Ping latency  : %u ms\n", report.pingLatencyMs);
    }

    analyzer.startContinuousSampling(5000);
    Serial.println("Continuous sampling started (every 5 s)");
}

void loop() {
    at.poll();

    if (analyzer.isSampling()) {
        static uint32_t t0 = millis();
        if (millis() - t0 > 5000) {
            t0 = millis();
            CellRecord rec;
            analyzer.sampleServingCell(rec);
            Serial.printf("[CELL] RSSI=%d  CellID=%d  Op=%s\n",
                          rec.rssi, rec.cellId, rec.operatorName);
        }
    }
}
