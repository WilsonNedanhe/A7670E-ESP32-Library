// Phase 6 — Voice Calls
// Dials a number, monitors call state, and auto-answers incoming calls.

#include <A7670E.h>

UART  uart;
ATClient    at(uart);
Call voice(at);

// ── Change to a real number to test outbound call ──
const char* CALL_NUMBER = "+1234567890";
bool        dialOnce    = true;

void onCallStateChanged(const CallInfo& info, void*) {
    const char* stateNames[] = {
        "IDLE","DIALING","RINGING","ACTIVE","HELD","BUSY","NO_ANSWER","FAILED"
    };
    Serial.printf("[VOICE] State: %s  Number: %s  Incoming: %s\n",
                  stateNames[(int)info.state],
                  info.number,
                  info.incoming ? "YES" : "NO");

    // Auto-answer incoming calls
    if (info.state == CallState::RINGING && info.incoming) {
        Serial.println("Auto-answering...");
        voice.answer();
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    uart.begin();
    uart.powerOn();
    delay(5000);
    at.sendCmd("ATE0");

    voice.begin();
    voice.onStateChange(onCallStateChanged);
    voice.setSpeakerVolume(7);
}

void loop() {
    at.poll();

    // Dial once after 5 s — comment out if you only want to test incoming
    static uint32_t t0 = millis();
    if (dialOnce && millis() - t0 > 5000) {
        dialOnce = false;
        Serial.printf("Dialing %s ...\n", CALL_NUMBER);
        voice.dial(CALL_NUMBER);
    }

    delay(100);
}
