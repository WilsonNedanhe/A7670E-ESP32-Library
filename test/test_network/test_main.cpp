// Hardware-in-the-loop test — requires a wired A7670E + antenna + SIM.
// Run with: pio test -e esp32dev -f test_network --upload-port <port>
//
// This suite is NOT run in CI (see .github/workflows/build.yml, which
// only compiles the library and examples). Network talks to real modem
// hardware, so it can't be faked the way test_at's sendSMS test is —
// this is registration state and signal-quality parsing against a live SIM.
//
// TODO(Wilson): fill in with Unity RUN_TEST cases once you've got a
// board wired up. Keep assertions behavioral (e.g. "registration
// reaches REGISTERED within N seconds"), not just "doesn't crash".

#include <unity.h>
#include <A7670E.h>

void setUp(void) {}
void tearDown(void) {}

void test_placeholder(void) {
    TEST_IGNORE_MESSAGE("Network hardware test not yet implemented — see file header.");
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_placeholder);
    UNITY_END();
}

void loop() {}
