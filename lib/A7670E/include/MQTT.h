#pragma once
#include "ATClient.h"

// Phase 9 — MQTT client (using A7670E built-in MQTT stack)

struct MQTTConfig {
    char    broker[128];
    uint16_t port        = 1883;
    char    clientId[64];
    char    username[64];
    char    password[64];
    uint16_t keepAlive   = 60;
    bool    cleanSession = true;
    bool    useTLS       = false;
    // Last Will
    char    willTopic[128];
    char    willPayload[128];
    int     willQos      = 0;
    bool    willRetain   = false;
};

using MQTTMessageCallback = void (*)(const char* topic, const char* payload,
                                     int payloadLen, void* ctx);

class MQTT {
public:
    explicit MQTT(ATClient& at);

    bool connect(const MQTTConfig& cfg,
                 uint32_t timeoutMs = AT_HTTP_TIMEOUT);
    bool disconnect();
    bool isConnected();

    bool publish(const char* topic, const char* payload,
                 int qos = 0, bool retain = false);
    bool publish(const char* topic, const uint8_t* payload,
                 size_t len, int qos = 0, bool retain = false);

    bool subscribe  (const char* topic, int qos = 0);
    bool unsubscribe(const char* topic);

    // Must be called in loop() to handle incoming messages
    void loop();

    void onMessage(MQTTMessageCallback cb, void* ctx = nullptr);

private:
    ATClient&          _at;
    bool                _connected = false;
    MQTTMessageCallback _cb  = nullptr;
    void*               _ctx = nullptr;

    static void _urcHandler(const char* urc, void* ctx);
};
