// A7670E Diagnostic Console
//
// Built on the actual library modules (UART/ATClient/DeviceInfo/Network/
// SMS/GPS/HTTP/MQTT), not raw AT strings — that's what gives you the
// watchdog-safe readLine(), the SMS buffer-overflow fix, and safe URC
// handling for free. See docs/architecture.md for the poll()/URC contract
// before you extend this file.
#include <A7670E.h>

UART      uart;
ATClient  at(uart);
DeviceInfo devInfo(at);
Network   net(at);
SMS       sms(at);
GPS       gps(at);
HTTP      http(at);
MQTT      mqtt(at);

// ---------------------------------------------------------------------
// Small input helpers. Blocking on Serial input is fine here — this is
// an interactive console, not a real-time loop — but note it does NOT
// call at.poll() while waiting, so a URC arriving while you're typing a
// number will sit queued until you're back in loop(). That's expected:
// see docs/architecture.md, poll() should only run outside a prompt.
// ---------------------------------------------------------------------
static String readLineBlocking() {
    while (!Serial.available()) { delay(5); }
    String s = Serial.readStringUntil('\n');
    s.trim();
    return s;
}

static String prompt(const char* label) {
    Serial.print(label);
    Serial.print(": ");
    return readLineBlocking();
}

static void banner(const char* title) {
    Serial.println();
    Serial.print("=== ");
    Serial.print(title);
    Serial.println(" ===");
}

static void printMenu() {
    Serial.println();
    Serial.println("===================================================");
    Serial.println("A7670E Diagnostic Console v1.0");
    Serial.println("===================================================");
    Serial.println("[Device]");
    Serial.println(" 1. Device Info");
    Serial.println(" 2. Firmware");
    Serial.println(" 3. IMEI");
    Serial.println(" 4. ICCID");
    Serial.println(" 5. IMSI");
    Serial.println("[Network]");
    Serial.println(" 6. Signal Strength");
    Serial.println(" 7. Cell ID");
    Serial.println(" 8. LTE Band");
    Serial.println(" 9. Network Registration");
    Serial.println("10. Operator");
    Serial.println("[SMS]");
    Serial.println("11. Read SMS");
    Serial.println("12. Send SMS");
    Serial.println("13. Delete SMS");
    Serial.println("[GPS]");
    Serial.println("14. Enable GPS");
    Serial.println("15. Current Position");
    Serial.println("16. Satellites");
    Serial.println("17. Speed");
    Serial.println("[Internet]");
    Serial.println("18. HTTP GET");
    Serial.println("19. MQTT Connect");
    Serial.println("20. Ping Server");
    Serial.println("[Diagnostics]");
    Serial.println("21. UART Monitor");
    Serial.println("22. Reset Modem");
    Serial.println("23. Sleep Mode");
    Serial.println("24. Power Off");
    Serial.println(" 0. Exit");
    Serial.println("---------------------------------------------------");
    Serial.print("Select option: ");
}

// GPS module doesn't cache the last fix (see GPS.h — getFix() is the only
// read), so "Satellites"/"Speed" re-request a fix rather than pretending
// to read something already known. Reusing the same call rather than a
// third variant avoids drifting out of sync with GPS.h if it changes.
static bool lastGoodFix(GNSSFix& fix) {
    if (!gps.getFix(fix) || !fix.valid) {
        Serial.println("No GPS fix available yet. Try option 14, then wait for a fix.");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------
// Setup / loop
// ---------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.printf("A7670E-ESP32-Library v%s\n", A7670E_LIB_VERSION);

    uart.begin();
    uart.powerOn();
    delay(5000);

    at.sendCmd("ATE0");
    sms.begin();

    printMenu();
}

void loop() {
    // Drains any queued URCs (incoming SMS, etc). Safe to call here —
    // never call it from inside a sendCmd()-based helper above.
    at.poll();

    if (!Serial.available()) return;

    String input = readLineBlocking();
    if (input.length() == 0) { printMenu(); return; }
    int choice = input.toInt();

    switch (choice) {

    // ---------------- Device ----------------
    case 1: {
        banner("Device Info");
        DeviceInfoData info;
        if (devInfo.getDeviceInfo(info)) {
            Serial.printf("Manufacturer: %s\n", info.manufacturer);
            Serial.printf("Model:        %s\n", info.model);
            Serial.printf("Firmware:     %s\n", info.firmware);
            Serial.printf("IMEI:         %s\n", info.imei);
            Serial.printf("IMSI:         %s\n", info.imsi);
            Serial.printf("ICCID:        %s\n", info.iccid);
        } else {
            Serial.println("Failed to read device info.");
        }
        break;
    }
    case 2: {
        banner("Firmware");
        char buf[48];
        if (devInfo.getFirmware(buf, sizeof(buf))) Serial.println(buf);
        else Serial.println("Failed to read firmware version.");
        break;
    }
    case 3: {
        banner("IMEI");
        char buf[20];
        if (devInfo.getIMEI(buf, sizeof(buf))) Serial.println(buf);
        else Serial.println("Failed to read IMEI.");
        break;
    }
    case 4: {
        banner("ICCID");
        char buf[24];
        if (devInfo.getICCID(buf, sizeof(buf))) Serial.println(buf);
        else Serial.println("Failed to read ICCID. (SIM inserted?)");
        break;
    }
    case 5: {
        banner("IMSI");
        char buf[20];
        if (devInfo.getIMSI(buf, sizeof(buf))) Serial.println(buf);
        else Serial.println("Failed to read IMSI. (SIM inserted?)");
        break;
    }

    // ---------------- Network ----------------
    case 6: {
        banner("Signal Strength");
        SignalInfo info;
        if (net.getSignalInfo(info)) {
            Serial.printf("CSQ:  %d (0-31, 99 = unknown)\n", info.csq);
            Serial.printf("RSSI: %d dBm\n", info.rssi);
            if (info.tech == NetTech::LTE || info.tech == NetTech::LTE_CAT_M) {
                Serial.printf("RSRP: %d dBm\n", info.rsrp);
                Serial.printf("RSRQ: %d dB\n",  info.rsrq);
                Serial.printf("SINR: %d dB\n",  info.sinr);
            }
        } else {
            Serial.println("Failed to read signal info.");
        }
        break;
    }
    case 7: {
        banner("Cell ID");
        SignalInfo info;
        if (net.getSignalInfo(info)) Serial.printf("Cell ID: %d  LAC: %d\n", info.cellId, info.lac);
        else Serial.println("Failed to read cell ID.");
        break;
    }
    case 8: {
        // No dedicated band field on SignalInfo yet (see roadmap.md) —
        // showing the raw AT+CPSI? response rather than inventing a
        // parsed value the library doesn't actually provide.
        banner("LTE Band (raw AT+CPSI?)");
        char resp[256];
        at.sendCmd("AT+CPSI?", "OK", AT_DEFAULT_TIMEOUT, resp, sizeof(resp));
        Serial.println(resp);
        break;
    }
    case 9: {
        banner("Network Registration");
        switch (net.getRegistrationStatus()) {
            case RegStatus::REGISTERED_HOME:    Serial.println("Registered (home)"); break;
            case RegStatus::REGISTERED_ROAMING: Serial.println("Registered (roaming)"); break;
            case RegStatus::SEARCHING:          Serial.println("Searching..."); break;
            case RegStatus::DENIED:             Serial.println("Registration denied"); break;
            case RegStatus::NOT_REGISTERED:     Serial.println("Not registered"); break;
            default:                            Serial.println("Unknown"); break;
        }
        break;
    }
    case 10: {
        banner("Operator");
        SignalInfo info;
        if (net.getSignalInfo(info)) Serial.println(info.operatorName);
        else Serial.println("Failed to read operator.");
        break;
    }

    // ---------------- SMS ----------------
    case 11: {
        banner("Read SMS");
        int idx = prompt("Message index").toInt();
        SMSMessage msg;
        if (sms.readSMS(idx, msg)) {
            Serial.printf("From: %s  [%s]\n", msg.sender, msg.status);
            Serial.printf("Time: %s\n", msg.timestamp);
            Serial.printf("Text: %s\n", msg.text);
        } else {
            Serial.println("Failed to read SMS at that index.");
        }
        break;
    }
    case 12: {
        banner("Send SMS");
        String number = prompt("Number (with country code, e.g. +263...)");
        String text   = prompt("Text");
        if (sms.sendSMS(number.c_str(), text.c_str())) Serial.println("Sent.");
        else Serial.println("Send failed.");
        break;
    }
    case 13: {
        banner("Delete SMS");
        int idx = prompt("Message index (or -1 for all)").toInt();
        bool ok = (idx < 0) ? sms.deleteAllSMS() : sms.deleteSMS(idx);
        Serial.println(ok ? "Deleted." : "Delete failed.");
        break;
    }

    // ---------------- GPS ----------------
    case 14: {
        banner("Enable GPS");
        // GPS::begin() is a no-op placeholder — powerOn() is what
        // actually issues AT+CGNSSPWR=1. See GPS.cpp.
        Serial.println(gps.powerOn() ? "GNSS powered on. Fix may take 30s-a few minutes outdoors."
                                      : "Failed to power on GNSS.");
        break;
    }
    case 15: {
        banner("Current Position");
        GNSSFix fix;
        if (lastGoodFix(fix)) {
            Serial.printf("Lat: %.6f  Lon: %.6f  Alt: %.1fm\n", fix.latitude, fix.longitude, fix.altitude);
            Serial.printf("UTC: %s\n", fix.utcTime);
        }
        break;
    }
    case 16: {
        banner("Satellites");
        GNSSFix fix;
        if (lastGoodFix(fix)) Serial.printf("Satellites in use: %d  HDOP: %.1f\n", fix.satellites, fix.hdop);
        break;
    }
    case 17: {
        banner("Speed");
        GNSSFix fix;
        if (lastGoodFix(fix)) Serial.printf("Speed: %.1f km/h  Course: %.1f deg\n", fix.speed, fix.course);
        break;
    }

    // ---------------- Internet ----------------
    case 18: {
        banner("HTTP GET");
        String url = prompt("URL");
        HTTPResponse resp;
        http.begin();
        bool ok = http.get(url.c_str(), resp);
        http.end();
        if (ok) {
            Serial.printf("Status: %d  Bytes: %d\n", resp.statusCode, resp.bodyLen);
            Serial.println(resp.body);
        } else {
            Serial.println("HTTP GET failed.");
        }
        break;
    }
    case 19: {
        banner("MQTT Connect");
        MQTTConfig cfg = {};
        String broker = prompt("Broker host");
        strncpy(cfg.broker, broker.c_str(), sizeof(cfg.broker) - 1);
        cfg.port = prompt("Port (e.g. 1883)").toInt();
        String clientId = prompt("Client ID");
        strncpy(cfg.clientId, clientId.c_str(), sizeof(cfg.clientId) - 1);
        Serial.println(mqtt.connect(cfg) ? "MQTT connected." : "MQTT connect failed.");
        break;
    }
    case 20: {
        banner("Ping Server");
        String host = prompt("Host (e.g. 8.8.8.8)");
        Serial.println(net.ping(host.c_str()) ? "Ping OK." : "Ping failed.");
        break;
    }

    // ---------------- Diagnostics ----------------
    case 21: {
        banner("UART Monitor (10s passthrough, Ctrl+] or wait to exit)");
        // Reads raw bytes directly off the UART instead of going through
        // ATClient — fine here because no sendCmd()/poll() runs
        // concurrently with this loop. Yields every idle pass so it
        // can't trip the watchdog the way the old sendAT() busy-wait did.
        uint32_t t0 = millis();
        while (millis() - t0 < 10000) {
            if (uart.available()) {
                Serial.write((char)uart.read());
            } else {
                delay(1);
            }
        }
        Serial.println("\n[monitor closed]");
        break;
    }
    case 22: {
        banner("Reset Modem");
        uart.hardReset();
        Serial.println("Hardware reset pulsed. Give it a few seconds to re-attach.");
        break;
    }
    case 23: {
        // Not implemented at the library level yet — A7670E sleep needs
        // DTR held low to stay awake, and the DTR line isn't wired up
        // by default (A7670E_DTR_PIN = -1, see Config.h / roadmap.md).
        // Sending AT+CSCLK=1 alone without DTR control will make the
        // module unresponsive until it's toggled again, so this is left
        // as an explicit no-op rather than a command that half-works.
        banner("Sleep Mode");
        Serial.println("Not supported yet: requires A7670E_DTR_PIN wired + DTR-aware");
        Serial.println("UART handling, neither of which exist in this library build.");
        Serial.println("See docs/roadmap.md.");
        break;
    }
    case 24: {
        banner("Power Off");
        uart.powerOff();
        Serial.println("Modem powered off.");
        break;
    }

    case 0: {
        // Arduino sketches can't actually exit loop() — "Exit" here means
        // stop processing console input, not terminate the program.
        banner("Exit");
        Serial.println("Console idle. Reset the board to restart it.");
        while (true) { at.poll(); delay(100); }
    }

    default:
        Serial.println("Invalid option.");
        break;
    }

    printMenu();
}
