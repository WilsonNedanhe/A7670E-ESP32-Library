// Phase 5 — SMS Send & Receive
// Sends a test SMS and prints every incoming message to Serial.

#include <A7670E.h>

UART uart;
ATClient   at(uart);
SMS  sms(at);

// ── Change these ──
const char* DEST_NUMBER = "+1234567890";

void onSMSReceived(const SMSMessage& msg, void*) {
    Serial.printf("\n[SMS] From: %s\n", msg.sender);
    Serial.printf("      Time: %s\n", msg.timestamp);
    Serial.printf("      Text: %s\n", msg.text);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    uart.begin();
    uart.powerOn();
    delay(5000);
    at.sendCmd("ATE0");

    sms.begin();
    sms.onReceived(onSMSReceived);

    Serial.printf("Sending SMS to %s ...\n", DEST_NUMBER);
    bool ok = sms.sendSMS(DEST_NUMBER, "Hello from A7670E-ESP32-Library!");
    Serial.println(ok ? "Sent!" : "Send failed");

    // Print unread messages already in SIM
    SMSMessage inbox[10];
    int n = sms.listUnread(inbox, 10);
    Serial.printf("%d unread message(s) in SIM\n", n);
    for (int i = 0; i < n; i++) {
        Serial.printf("  [%d] From:%s  %s\n", inbox[i].index,
                      inbox[i].sender, inbox[i].text);
    }
}

void loop() {
    at.poll();  // triggers onSMSReceived when +CMTI arrives
    delay(100);
}
