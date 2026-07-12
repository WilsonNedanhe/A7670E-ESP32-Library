// Host-side (native) test for the sendSMS() buffer bug fix.
// Run with: pio test -e native
//
// This exercises the REAL library code (UART -> ATClient -> SMS), not a
// reimplementation, using a fake HardwareSerial that scripts AT responses
// and records exactly what SMS::sendSMS() writes to the wire. That's the
// only way to actually prove the 1-byte overflow (see A7670E_SMS bug in
// the review) can't happen anymore, rather than just re-asserting it by
// inspection.

#include <unity.h>
#include <string>
#include <cstring>
#include "Config.h"
#include "UART.h"
#include "ATClient.h"
#include "SMS.h"

// Fake modem: replies "\r\n> \r\n" to the CMGS prompt command, then
// "\r\n+CMGS: 1\r\n\r\nOK\r\n" to whatever raw body arrives afterward.
// All writes (including the raw SMS body) are captured for inspection.
class FakeModem : public HardwareSerial {
public:
    std::string rxQueue;   // bytes waiting to be "read" by the library
    std::string lastWrite; // last thing written via write(buf,len)
    int callCount = 0;

    int available() override { return (int)rxQueue.size(); }

    int read() override {
        if (rxQueue.empty()) return -1;
        char c = rxQueue.front();
        rxQueue.erase(rxQueue.begin());
        return (unsigned char)c;
    }

    size_t print(const char* s) override {
        // AT command text (e.g. AT+CMGS="...") — first call gets the
        // '>' prompt queued, subsequent commands aren't used in this test.
        if (callCount++ == 0) rxQueue = "\r\n> \r\n";
        return strlen(s);
    }

    size_t write(const uint8_t* buf, size_t len) override {
        lastWrite.assign((const char*)buf, len);
        // Simulate the modem accepting the SMS body.
        rxQueue = "\r\n+CMGS: 1\r\n\r\nOK\r\n";
        return len;
    }
};

void setUp(void) {}
void tearDown(void) {}

// text length == SMS_MAX_LEN (160): last valid, non-truncated case.
void test_sendSMS_exact_max_len_no_overflow(void) {
    FakeModem modem;
    UART uart(modem, -1, -1, -1, -1);
    ATClient at(uart);
    SMS sms(at);

    std::string text(SMS_MAX_LEN, 'x'); // exactly 160 chars
    bool ok = sms.sendSMS("+1234567890", text.c_str());

    TEST_ASSERT_TRUE(ok);
    // Wire payload must be exactly text + 0x1A (161 bytes) — not truncated,
    // not overflowed.
    TEST_ASSERT_EQUAL_UINT(SMS_MAX_LEN + 1, modem.lastWrite.size());
    TEST_ASSERT_EQUAL_UINT8(0x1A, (uint8_t)modem.lastWrite[SMS_MAX_LEN]);
}

// text length > SMS_MAX_LEN: this is the exact input that overflowed
// body[SMS_MAX_LEN + 2] by one byte in the original code.
void test_sendSMS_over_max_len_clamped_not_overflowed(void) {
    FakeModem modem;
    UART uart(modem, -1, -1, -1, -1);
    ATClient at(uart);
    SMS sms(at);

    std::string text(SMS_MAX_LEN + 40, 'y'); // 200 chars, well over the limit
    bool ok = sms.sendSMS("+1234567890", text.c_str());

    TEST_ASSERT_TRUE(ok);
    // Must be clamped to SMS_MAX_LEN + terminator, never longer.
    TEST_ASSERT_EQUAL_UINT(SMS_MAX_LEN + 1, modem.lastWrite.size());
    TEST_ASSERT_EQUAL_UINT8(0x1A, (uint8_t)modem.lastWrite[SMS_MAX_LEN]);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_sendSMS_exact_max_len_no_overflow);
    RUN_TEST(test_sendSMS_over_max_len_clamped_not_overflowed);
    return UNITY_END();
}
