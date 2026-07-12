#pragma once
#include <Arduino.h>

// ─── Hardware pin defaults (override in your sketch before including A7670E.h) ───
#ifndef A7670E_TX_PIN
#define A7670E_TX_PIN       17
#endif
#ifndef A7670E_RX_PIN
#define A7670E_RX_PIN       16
#endif
#ifndef A7670E_PWRKEY_PIN
#define A7670E_PWRKEY_PIN   4
#endif
#ifndef A7670E_RESET_PIN
#define A7670E_RESET_PIN    5
#endif
#ifndef A7670E_DTR_PIN
#define A7670E_DTR_PIN      -1   // -1 = not connected
#endif

// ─── Serial port ───
#ifndef A7670E_SERIAL
#define A7670E_SERIAL       Serial2
#endif
#ifndef A7670E_BAUD
#define A7670E_BAUD         115200
#endif

// ─── Timeouts (ms) ───
#define AT_DEFAULT_TIMEOUT  3000
#define AT_LONG_TIMEOUT     30000
#define AT_NETWORK_TIMEOUT  60000
#define AT_GPS_TIMEOUT      120000
#define AT_HTTP_TIMEOUT     60000

// ─── Buffer sizes ───
#define AT_RX_BUF_SIZE      1024
#define AT_TX_BUF_SIZE      512
#define SMS_MAX_LEN         160
#define HTTP_RESP_BUF_SIZE  4096

// ─── Debug output ───
#ifndef A7670E_DEBUG
#define A7670E_DEBUG        0
#endif
#if A7670E_DEBUG
#define DBG(...)            Serial.printf("[A7670E] " __VA_ARGS__)
#else
#define DBG(...)
#endif
