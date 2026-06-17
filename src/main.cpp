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
// Pass the MOTOR_DUTY_LIMIT directly to the constructors to solve compiling issues
MotorController motorLeft(PIN_MOTOR_L_IN1, PIN_MOTOR_L_IN2, 0, 1, MOTOR_DUTY_LIMIT);
MotorController motorRight(PIN_MOTOR_R_IN1, PIN_MOTOR_R_IN2, 2, 3, MOTOR_DUTY_LIMIT);

MagneticEncoder encoderLeft(Wire1, MUX_I2C_ADDR, MUX_CHAN_LEFT_ENCODER);
MagneticEncoder encoderRight(Wire1, MUX_I2C_ADDR, MUX_CHAN_RIGHT_ENCODER);

BMI160_Custom imu(Wire1, IMU_I2C_ADDR);

// Instantiate global thread-safe communication states
RobotState g_robotState = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
std::mutex g_stateMutex;

// Defensive helper to print to both Native USB CDC and Hardware UART0
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
    // -------------------------------------------------------------------------
    // CRITICAL FIRST LINE DEFENSE: Kill floating states on motor driver pins
    // -------------------------------------------------------------------------
    pinMode(PIN_MOTOR_L_IN1, OUTPUT);
    pinMode(PIN_MOTOR_L_IN2, OUTPUT);
    pinMode(PIN_MOTOR_R_IN1, OUTPUT);
    pinMode(PIN_MOTOR_R_IN2, OUTPUT);
    digitalWrite(PIN_MOTOR_L_IN1, LOW);
    digitalWrite(PIN_MOTOR_L_IN2, LOW);
    digitalWrite(PIN_MOTOR_R_IN1, LOW);
    digitalWrite(PIN_MOTOR_R_IN2, LOW);

    // Initialize serial ports
    Serial.begin(115200);   // Native USB CDC
    Serial0.begin(115200);  // CP2102 Hardware UART0
    
    delay(2000); 
    systemLog("\n[SYSTEM] Initialize Anjoman ESP32 Swarm Robot...\n");

    // Initialize I2C0 Bus (ToF Dedicated Bus at 100kHz)
    Wire.begin(PIN_I2C0_SDA, PIN_I2C0_SCL, 100000U);
    Wire.setTimeOut(50); // Set 50ms timeout to prevent hardware lockups [1]
    systemLog("[SYSTEM] Hardware Bus I2C0 initialized at 100kHz.\n");

    // Initialize I2C1 Bus (Shared IMU & Multiplexer Bus at 400kHz)
    Wire1.begin(PIN_I2C1_SDA, PIN_I2C1_SCL, 400000U);
    Wire1.setTimeOut(50); // Set 50ms timeout to prevent hardware lockups [1]
    systemLog("[SYSTEM] Hardware Bus I2C1 initialized at 400kHz.\n");

    // Initialize Motor Drivers
    if (motorLeft.begin() && motorRight.begin()) {
        systemLog("[SYSTEM] DRV8833 Motor Drivers initialized and secured.\n");
    } else {
        systemLog("[CRITICAL ERROR] Failed to initialize DRV8833 Motor Controllers.\n");
    }

    // Initialize Magnetic Encoders
    systemLog("[SYSTEM] Attempting communication with AS5600 Encoders...\n");
    if (encoderLeft.begin() && encoderRight.begin()) {
        systemLog("[SYSTEM] AS5600 Magnetic Encoders initialized on I2C1.\n");
    } else {
        systemLog("[WARNING] Failed to detect Encoders on Multiplexer I2C1. Check physical pull-ups.\n");
    }

    // Initialize Bosch BMI160 IMU (Now isolated behind Mux Channel 3)
    systemLog("[SYSTEM] Attempting communication with BMI160 IMU behind Channel 3...\n");
    if (imu.begin()) {
        imu.configureDefault();
        systemLog("[SYSTEM] Bosch BMI160 IMU detected and configured via Multiplexer.\n");
    } else {
        systemLog("[WARNING] Failed to communicate with BMI160 IMU. Bus timeout bypassed.\n");
    }

    // Create FreeRTOS Tasks and pin them to their target cores
    startTelemetryTask(); // Spawns on Core 0 (WiFi + TCP + Terminal logs)
    startControlTask();   // Spawns on Core 1 (Physical motor logic at 100Hz)

    systemLog("[SYSTEM] Setup completed. FreeRTOS Scheduler running.\n");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
