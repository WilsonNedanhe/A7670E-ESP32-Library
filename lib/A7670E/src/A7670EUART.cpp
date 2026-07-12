#include "A7670EUART.h"

A7670EUART::A7670EUART(HardwareSerial& serial)
{
    _serial = &serial;
}

bool A7670EUART::begin(uint32_t baud,
                       int8_t rxPin,
                       int8_t txPin)
{
    _serial->begin(baud, SERIAL_8N1, rxPin, txPin);

    delay(500);

    return true;
}

void A7670EUART::send(const String& command)
{
    _serial->println(command);
}

bool A7670EUART::available()
{
    return _serial->available();
}

String A7670EUART::readString()
{
    return _serial->readString();
}

void A7670EUART::flush()
{
    while(_serial->available())
        _serial->read();
}