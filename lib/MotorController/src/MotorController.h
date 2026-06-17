#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <Arduino.h>

class MotorController {
public:
    // Links physical direction and PWM pins of DRV8833
    MotorController(uint8_t pinIn1, uint8_t pinIn2, uint8_t ledcChan1, uint8_t ledcChan2);

    // Configures hardware timers for high frequency (20kHz to prevent acoustic coil hum)
    bool begin();

    // Sets velocity from -1.0 (full reverse) to 1.0 (full forward)
    void setSpeed(float speed);

    // Forces motor inputs into fast-decay braking mode
    void brake();

private:
    uint8_t _pinIn1;
    uint8_t _pinIn2;
    uint8_t _chan1;
    uint8_t _chan2;
    
    const uint32_t PWM_FREQ = 20000; // 20 kHz safe ultrasonic PWM frequency
    const uint8_t PWM_RES_BITS = 10; // 10-bit resolution (0 to 1023 duty cycle)
    const uint32_t MAX_DUTY = 1023;
};

#endif // MOTOR_CONTROLLER_H
