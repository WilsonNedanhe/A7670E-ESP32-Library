// Phase 12 — Machine Learning Dataset Generator
// Collects labelled (cell + GPS + time) feature vectors and writes CSV / JSONL.

#include <A7670E.h>

#define SD_CS_PIN 5

A7670E_UART            uart;
A7670E_AT              at(uart);
A7670E_Network         net(at);
A7670E_GNSS            gnss(at);
A7670E_Logger          logger(SD_CS_PIN);
A7670E_TelecomAnalyzer analyzer(at, net, &logger);
A7670E_MLDataGen       mlgen(analyzer, gnss, logger);

// Label set — cycle through or change manually
const char* LABELS[] = { "indoor", "outdoor", "moving", "stationary" };
int         labelIdx  = 0;

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

    gnss.begin();
    gnss.powerOn();

    Serial.println("ML Dataset Generator ready.");
    Serial.println("Press ENTER to collect a sample, 'l' to cycle label.");
}

void loop() {
    at.poll();

    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'l' || c == 'L') {
            labelIdx = (labelIdx + 1) % 4;
            Serial.printf("Label set to: %s\n", LABELS[labelIdx]);
        } else if (c == '\r' || c == '\n' || c == ' ') {
            Serial.printf("Collecting sample [label=%s] ...\n", LABELS[labelIdx]);

            MLSample sample;
            if (mlgen.collectSample(sample, LABELS[labelIdx], 1.0f, 10000)) {
                Serial.printf("  RSSI=%d  lat=%.5f  lon=%.5f  gpsOk=%d\n",
                              sample.rssi, sample.latitude,
                              sample.longitude, sample.gpsValid);

                mlgen.appendCSV  (sample, "/ml_dataset.csv");
                mlgen.appendJSONL(sample, "/ml_dataset.jsonl");
                Serial.println("  → Written to SD");
            } else {
                Serial.println("  Collection failed");
            }
        }
    }
}
