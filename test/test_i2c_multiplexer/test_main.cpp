#include <Arduino.h>
#include <Wire.h>
#include <unity.h>

const uint8_t MUX_ADDR = 0x70;
const uint8_t PIN_SDA_I2C1 = 1;
const uint8_t PIN_SCL_I2C1 = 2;

void test_multiplexer_connection(void) {
    Wire1.beginTransmission(MUX_ADDR);
    uint8_t error = Wire1.endTransmission();
    TEST_ASSERT_EQUAL_UINT8(0, error);
}

void test_channel_selection_logic(void) {
    for (uint8_t channel = 0; channel < 3; channel++) {
        Wire1.beginTransmission(MUX_ADDR);
        Wire1.write(1 << channel);
        uint8_t error = Wire1.endTransmission();
        TEST_ASSERT_EQUAL_UINT8_08(0, error);
    }
}

void setup() {
    delay(2000);
    Wire1.begin(PIN_SDA_I2C1, PIN_SCL_I2C1, 400000U); // 400kHz Fast I2C1
    
    UNITY_BEGIN();
    RUN_TEST(test_multiplexer_connection);
    RUN_TEST(test_channel_selection_logic);
    UNITY_END();
}

void loop() {
}
