#include <Arduino.h>
#include <Wire.h>
#include "PinMap.h"
#include "SystemConfig.h"
#include "SharedState.h"
#include "MotorController.h"
#include "MagneticEncoder.h"
#include "BMI160_Custom.h"
#include "ControlTask.h"
#include "TelemetryTask.h"

// Instantiate physical hardware driver objects globally with OOP decoupling
// Set dynamic defaults: 5kHz frequency, 10-bit resolution
MotorController motorLeft(PIN_MOTOR_L_IN1, PIN_MOTOR_L_IN2, 0, 1);
MotorController motorRight(PIN_MOTOR_R_IN1, PIN_MOTOR_R_IN2, 2, 3);
MotorController motorSweep(PIN_SWEEP_IN1, PIN_SWEEP_IN2, 4, 5);

MagneticEncoder encoderLeft(Wire1, MUX_I2C_ADDR, MUX_CHAN_LEFT_ENCODER);
MagneticEncoder encoderRight(Wire1, MUX_I2C_ADDR, MUX_CHAN_RIGHT_ENCODER);
MagneticEncoder encoderSweep(Wire1, MUX_I2C_ADDR, MUX_CHAN_SWEEP_ENCODER);

BMI160_Custom imu(Wire1, IMU_I2C_ADDR);

void systemLog(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.print(buffer);
    Serial0.print(buffer);
}

void setup() {
    // Force immediate safe state to prevent motor floating on startup
    pinMode(PIN_MOTOR_L_IN1, OUTPUT);
    pinMode(PIN_MOTOR_L_IN2, OUTPUT);
    pinMode(PIN_MOTOR_R_IN1, OUTPUT);
    pinMode(PIN_MOTOR_R_IN2, OUTPUT);
    pinMode(PIN_SWEEP_IN1, OUTPUT);
    pinMode(PIN_SWEEP_IN2, OUTPUT);

    digitalWrite(PIN_MOTOR_L_IN1, LOW);
    digitalWrite(PIN_MOTOR_L_IN2, LOW);
    digitalWrite(PIN_MOTOR_R_IN1, LOW);
    digitalWrite(PIN_MOTOR_R_IN2, LOW);
    digitalWrite(PIN_SWEEP_IN1, LOW);
    digitalWrite(PIN_SWEEP_IN2, LOW);

    Serial.begin(115200);   
    Serial0.begin(115200);  
    
    delay(2000); 
    systemLog("\n==================================================\n");
    systemLog("[ANJOMAN BRINGUP] System initialized.\n");
    systemLog("==================================================\n");

    // Initialize Wire1 at 400kHz Fast-mode
    Wire1.begin(PIN_I2C1_SDA, PIN_I2C1_SCL, 400000U);
    Wire1.setTimeOut(50); 

    // Initial default execution values passed down to HAL
    motorLeft.begin(5000, 10, MOTOR_DUTY_LIMIT);
    motorRight.begin(5000, 10, MOTOR_DUTY_LIMIT);
    motorSweep.begin(5000, 10, MOTOR_DUTY_LIMIT);

    encoderLeft.begin();
    encoderRight.begin();
    encoderSweep.begin();

    // BMI160 initialization (No offsets applied in this bringup phase)
    if (imu.begin()) {
        imu.configureDefault();
        systemLog("[SYSTEM] Bosch BMI160 online.\n");
    } else {
        systemLog("[WARNING] BMI160 Offline. Bypass.\n");
    }

    startTelemetryTask(); // Web server on Core 0
    startControlTask();   // Dynamic motor/encoder HAL on Core 1

    systemLog("[SYSTEM] Scheduler initiated. Navigate to http://192.168.1.150\n");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
