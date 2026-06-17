#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <Arduino.h>

class MotorController {
public:
    // Added dutyLimit parameter to constructor to decouple from project-specific headers
    MotorController(uint8_t pinIn1, uint8_t pinIn2, uint8_t ledcChan1, uint8_t ledcChan2, float dutyLimit = 1.0f);

    bool begin();
    void setSpeed(float speed);
    void brake();

private:
    uint8_t _pinIn1;
    uint8_t _pinIn2;
    uint8_t _chan1;
    uint8_t _chan2;
    float _dutyLimit; // Caches the electrical limit applied to the PWM output
    
    const uint32_t PWM_FREQ = 5000; 
    const uint8_t PWM_RES_BITS = 10; 
    const uint32_t MAX_DUTY = 1023;
};

#endif // MOTOR_CONTROLLER_H
