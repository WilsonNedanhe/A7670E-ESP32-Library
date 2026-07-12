// Phase 9 — MQTT Publish & Subscribe
// Connects to broker, subscribes to a topic, publishes every 10 s.

#include <A7670E.h>

A7670E_UART    uart;
A7670E_AT      at(uart);
A7670E_Network net(at);
A7670E_MQTT    mqtt(at);

// ── Configure your broker ──
MQTTConfig cfg = {
    .broker      = "broker.hivemq.com",
    .port        = 1883,
    .clientId    = "ESP32_A7670E_001",
    .username    = "",
    .password    = "",
    .keepAlive   = 60,
    .cleanSession = true,
};

void onMQTTMessage(const char* topic, const char* payload, int len, void*) {
    Serial.printf("[MQTT] %s → %.*s\n", topic, len, payload);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    uart.begin();
    uart.powerOn();
    delay(5000);
    at.sendCmd("ATE0");

    net.setAPN("internet");
    net.activate(30000);

    Serial.println("Connecting to MQTT broker...");
    if (!mqtt.connect(cfg, 30000)) {
        Serial.println("MQTT connect failed"); return;
    }
    Serial.println("Connected!");

    mqtt.onMessage(onMQTTMessage);
    mqtt.subscribe("esp32/cmd", 1);
    Serial.println("Subscribed to esp32/cmd");
}

void loop() {
    mqtt.loop();

    static uint32_t t0 = millis();
    if (millis() - t0 > 10000) {
        t0 = millis();
        char payload[64];
        snprintf(payload, sizeof(payload),
                 "{\"uptime\":%lu,\"heap\":%u}",
                 millis() / 1000, (unsigned)ESP.getFreeHeap());
        Serial.printf("Publishing: %s\n", payload);
        mqtt.publish("esp32/telemetry", payload, 1);
    }
}
