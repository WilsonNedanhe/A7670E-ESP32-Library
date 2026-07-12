#pragma once
// Minimal Arduino shim for `pio test -e native`.
// Only implements what UART.cpp / ATClient.cpp / SMS.cpp actually touch.
// This lets the SMS/AT buffer-handling logic run as a real host-side unit
// test instead of only ever being exercised on physical hardware.
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <chrono>

#define HIGH 1
#define LOW  0
#define OUTPUT 1
#define SERIAL_8N1 0

typedef unsigned char uint8_t_local; // avoid clashing with <cstdint> uint8_t

inline uint32_t millis() {
    using namespace std::chrono;
    static auto t0 = steady_clock::now();
    return (uint32_t)duration_cast<milliseconds>(steady_clock::now() - t0).count();
}
inline void delay(uint32_t) {}
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}

// Fake serial: tests subclass this to script responses / capture writes.
class HardwareSerial {
public:
    virtual void begin(uint32_t, int, int, int) {}
    virtual int  available() { return 0; }
    virtual int  read() { return -1; }
    virtual size_t print(const char*) { return 0; }
    virtual size_t write(const uint8_t*, size_t len) { return len; }
    virtual void flush() {}
    virtual ~HardwareSerial() {}
};

// Config.h's A7670E_SERIAL macro defaults to Serial2; tests construct
// UART with an explicit fake serial instead, but the default arg still
// needs *some* symbol to resolve against at parse time.
class NullSerial : public HardwareSerial {};
static NullSerial Serial2;

// --- minimal extras for compile-checking sketches natively (not real Arduino) ---
#include <string>
#include <cstdio>
#include <cstdarg>

class String {
public:
    std::string s;
    String() {}
    String(const char* c) : s(c ? c : "") {}
    String(int v) { s = std::to_string(v); }
    void trim() {
        size_t a = s.find_first_not_of(" \t\r\n");
        size_t b = s.find_last_not_of(" \t\r\n");
        s = (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
    }
    int toInt() const { return s.empty() ? 0 : atoi(s.c_str()); }
    size_t length() const { return s.size(); }
    const char* c_str() const { return s.c_str(); }
};

class SerialClass : public HardwareSerial {
public:
    void begin(uint32_t) {}
    template<typename... Args> void printf(const char* fmt, Args... args) { std::printf(fmt, args...); }
    size_t print(const char* s) override { return std::fputs(s, stdout); }
    void println(const char* s) { std::fputs(s, stdout); std::fputc('\n', stdout); }
    void println() { std::fputc('\n', stdout); }
    void write(char c) { std::fputc(c, stdout); }
    String readStringUntil(char) { return String(""); }
};
static SerialClass Serial;
