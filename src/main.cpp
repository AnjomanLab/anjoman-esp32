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

// Instantiate physical hardware driver objects globally
MotorController motorLeft(PIN_MOTOR_L_IN1, PIN_MOTOR_L_IN2, 0, 1);
MotorController motorRight(PIN_MOTOR_R_IN1, PIN_MOTOR_R_IN2, 2, 3);

MagneticEncoder encoderLeft(Wire1, MUX_I2C_ADDR, MUX_CHAN_LEFT_ENCODER);
MagneticEncoder encoderRight(Wire1, MUX_I2C_ADDR, MUX_CHAN_RIGHT_ENCODER);

BMI160_Custom imu(Wire1, IMU_I2C_ADDR);

// Instantiate global thread-safe communication states
RobotState g_robotState = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
std::mutex g_stateMutex;

void setup() {
    // Initiate Serial console. Port is automatically routed to USB CDC based on platformio.ini flags.
    Serial.begin(115200);
    delay(2000); 
    Serial.println("\n[SYSTEM] Initialize Anjoman ESP32 Swarm Robot...");

    // Initialize I2C0 Bus (ToF Dedicated Bus at 100kHz)
    Wire.begin(PIN_I2C0_SDA, PIN_I2C0_SCL, 100000U);
    Serial.println("[SYSTEM] Hardware Bus I2C0 initialized at 100kHz.");

    // Initialize I2C1 Bus (Shared IMU & Multiplexer Bus at 400kHz)
    Wire1.begin(PIN_I2C1_SDA, PIN_I2C1_SCL, 400000U);
    Serial.println("[SYSTEM] Hardware Bus I2C1 initialized at 400kHz.");

    // Initialize Motor Drivers
    if (motorLeft.begin() && motorRight.begin()) {
        Serial.println("[SYSTEM] DRV8833 Motor Drivers initialized.");
    } else {
        Serial.println("[CRITICAL ERROR] Failed to initialize DRV8833 Motor Controllers.");
    }

    // Initialize Magnetic Encoders
    if (encoderLeft.begin() && encoderRight.begin()) {
        Serial.println("[SYSTEM] AS5600 Magnetic Encoders initialized on I2C1.");
    } else {
        Serial.println("[CRITICAL ERROR] Failed to detect Encoders on Multiplexer I2C1.");
    }

    // Initialize Bosch BMI160 IMU
    if (imu.begin()) {
        imu.configureDefault();
        Serial.println("[SYSTEM] Bosch BMI160 IMU detected and configured.");
    } else {
        Serial.println("[CRITICAL ERROR] Failed to communicate with BMI160 IMU.");
    }

    // Create FreeRTOS Tasks and pin them to their target cores
    startTelemetryTask(); // Spawns on Core 0
    startControlTask();   // Spawns on Core 1

    Serial.println("[SYSTEM] Setup completed. FreeRTOS Scheduler running.");
}

void loop() {
    // Keep main thread suspended to save resources. Real-time tasks handle processing.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
