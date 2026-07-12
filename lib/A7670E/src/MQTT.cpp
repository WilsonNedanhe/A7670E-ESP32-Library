#include "MQTT.h"
#include <string.h>
#include <stdio.h>

MQTT::MQTT(ATClient& at) : _at(at) {}

bool MQTT::connect(const MQTTConfig& cfg, uint32_t timeoutMs) {
    char cmd[256];

    // Broker address + port
    snprintf(cmd, sizeof(cmd), "AT+CMQTTACCQ=0,\"%s\"", cfg.clientId);
    if (_at.sendCmd(cmd) != ATResult::OK) return false;

    // TLS
    if (cfg.useTLS) {
        _at.sendCmd("AT+CMQTTSSL=0,1");
    }

    // Connect
    snprintf(cmd, sizeof(cmd), "AT+CMQTTCONNECT=0,\"%s:%u\",%u,%d,\"%s\",\"%s\"",
             cfg.broker, cfg.port, cfg.keepAlive, cfg.cleanSession ? 1 : 0,
             cfg.username, cfg.password);

    if (_at.sendCmd(cmd, "+CMQTTCONNECT:", timeoutMs) != ATResult::OK) return false;

    // Register URC for incoming messages
    _at.registerURC("+CMQTTRXSTART:", _urcHandler, this);
    _connected = true;
    return true;
}

bool MQTT::disconnect() {
    bool ok = _at.sendCmd("AT+CMQTTDISC=0,120", "+CMQTTDISC:", AT_HTTP_TIMEOUT) == ATResult::OK;
    _at.sendCmd("AT+CMQTTREL=0");
    _connected = false;
    return ok;
}

bool MQTT::isConnected() {
    return _connected;
}

bool MQTT::publish(const char* topic, const char* payload,
                           int qos, bool retain) {
    return publish(topic, (const uint8_t*)payload, strlen(payload), qos, retain);
}

bool MQTT::publish(const char* topic, const uint8_t* payload,
                           size_t len, int qos, bool retain) {
    char cmd[128];
    // Set topic
    snprintf(cmd, sizeof(cmd), "AT+CMQTTTOPIC=0,%u", (unsigned)strlen(topic));
    if (_at.sendCmd(cmd, ">", AT_DEFAULT_TIMEOUT) != ATResult::OK) return false;
    _at.sendRaw((const uint8_t*)topic, strlen(topic), "OK", AT_DEFAULT_TIMEOUT);

    // Set payload
    snprintf(cmd, sizeof(cmd), "AT+CMQTTPAYLOAD=0,%u", (unsigned)len);
    if (_at.sendCmd(cmd, ">", AT_DEFAULT_TIMEOUT) != ATResult::OK) return false;
    _at.sendRaw(payload, len, "OK", AT_DEFAULT_TIMEOUT);

    // Publish
    snprintf(cmd, sizeof(cmd), "AT+CMQTTPUB=0,%d,60,%d", qos, retain ? 1 : 0);
    return _at.sendCmd(cmd, "+CMQTTPUB:", AT_HTTP_TIMEOUT) == ATResult::OK;
}

bool MQTT::subscribe(const char* topic, int qos) {
    char cmd[160];
    snprintf(cmd, sizeof(cmd), "AT+CMQTTSUB=0,\"%s\",%d", topic, qos);
    return _at.sendCmd(cmd, "+CMQTTSUB:", AT_HTTP_TIMEOUT) == ATResult::OK;
}

bool MQTT::unsubscribe(const char* topic) {
    char cmd[160];
    snprintf(cmd, sizeof(cmd), "AT+CMQTTUNSUB=0,\"%s\",0", topic);
    return _at.sendCmd(cmd, "+CMQTTUNSUB:", AT_DEFAULT_TIMEOUT) == ATResult::OK;
}

void MQTT::loop() {
    _at.poll();
}

void MQTT::onMessage(MQTTMessageCallback cb, void* ctx) {
    _cb  = cb;
    _ctx = ctx;
}

void MQTT::_urcHandler(const char* urc, void* ctx) {
    MQTT* self = static_cast<MQTT*>(ctx);
    if (!self->_cb) return;
    // Minimal URC parsing — topic and payload arrive in subsequent lines via poll
    // A full implementation would accumulate +CMQTTRXTOPIC and +CMQTTRXPAYLOAD URCs
    self->_cb("", urc, strlen(urc), self->_ctx);
}
