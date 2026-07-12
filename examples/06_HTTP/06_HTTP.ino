// Phase 8 — HTTP GET & POST
// Fetches a JSON endpoint and posts sensor data.

#include <A7670E.h>

A7670E_UART    uart;
A7670E_AT      at(uart);
A7670E_Network net(at);
A7670E_HTTP    http(at);

void setup() {
    Serial.begin(115200);
    delay(1000);

    uart.begin();
    uart.powerOn();
    delay(5000);
    at.sendCmd("ATE0");

    net.setAPN("internet");  // ← your APN
    Serial.println("Connecting to LTE...");
    net.activate(30000);

    http.begin();

    // ── GET ──
    Serial.println("\n--- HTTP GET ---");
    HTTPResponse resp;
    if (http.get("http://httpbin.org/get", resp)) {
        Serial.printf("Status: %d\n", resp.statusCode);
        Serial.printf("Body (first 200 chars):\n%.200s\n", resp.body);
    } else {
        Serial.printf("GET failed: %d\n", resp.statusCode);
    }

    // ── POST JSON ──
    Serial.println("\n--- HTTP POST ---");
    const char* payload = "{\"device\":\"ESP32\",\"rssi\":-65}";
    if (http.post("http://httpbin.org/post",
                  payload, strlen(payload),
                  "application/json", resp)) {
        Serial.printf("Status: %d\n", resp.statusCode);
        Serial.printf("Body (first 200 chars):\n%.200s\n", resp.body);
    }

    http.end();
}

void loop() {
    at.poll();
}
