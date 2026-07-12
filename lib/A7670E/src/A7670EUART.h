#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

class A7670EUART
{
public:

    A7670EUART(HardwareSerial& serial);

    bool begin(
        uint32_t baud = 115200,
        int8_t rxPin = 16,
        int8_t txPin = 17
    );

    void send(const String& command);

    bool available();

    String readString();

    void flush();

private:

    HardwareSerial* _serial;
};