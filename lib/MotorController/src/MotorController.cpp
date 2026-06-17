#include "MotorController.h"

MotorController::MotorController(uint8_t pinIn1, uint8_t pinIn2, uint8_t ledcChan1, uint8_t ledcChan2)
    : _pinIn1(pinIn1), _pinIn2(pinIn2), _chan1(ledcChan1), _chan2(ledcChan2) {}

bool MotorController::begin() {
    pinMode(_pinIn1, OUTPUT);
    pinMode(_pinIn2, OUTPUT);

    // Initialize high speed LEDC channels on ESP32-S3
    ledcSetup(_chan1, PWM_FREQ, PWM_RES_BITS);
    ledcSetup(_chan2, PWM_FREQ, PWM_RES_BITS);

    ledcAttachPin(_pinIn1, _chan1);
    ledcAttachPin(_pinIn2, _chan2);

    brake();
    return true;
}

void MotorController::setSpeed(float speed) {
    // Constrain input velocity values to protect motor windings from saturated spikes
    speed = constrain(speed, -1.0f, 1.0f);

    if (speed > 0.01f) {
        uint32_t duty = (uint32_t)(speed * (float)MAX_DUTY);
        ledcWrite(_chan1, duty);
        ledcWrite(_chan2, 0);
    } else if (speed < -0.01f) {
        uint32_t duty = (uint32_t)(-speed * (float)MAX_DUTY);
        ledcWrite(_chan1, 0);
        ledcWrite(_chan2, duty);
    } else {
        brake();
    }
}

void MotorController::brake() {
    // DRV8833 outputs low-impedance fast decay when both inputs are pulled high
    ledcWrite(_chan1, MAX_DUTY);
    ledcWrite(_chan2, MAX_DUTY);
}
