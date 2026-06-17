#include "ControlTask.h"
#include "PinMap.h"
#include "SystemConfig.h"
#include "SharedState.h"
#include "MotorController.h"
#include "MagneticEncoder.h"
#include "BMI160_Custom.h"

// Access global hardware driver instances declared in main.cpp
extern MotorController motorLeft;
extern MotorController motorRight;
extern MagneticEncoder encoderLeft;
extern MagneticEncoder encoderRight;
extern BMI160_Custom imu;

void controlTaskLoop(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (true) {
        // Read IMU raw values safely
        imu.readSensorData();
        
        // Read Encoders utilizing I2C1 bus mutexes
        float angleL = encoderLeft.getAngleRadians();
        float angleR = encoderRight.getAngleRadians();
        
        // Critical Section: Safely update shared global state
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            g_robotState.imuAccX = imu.getAccX();
            g_robotState.imuAccY = imu.getAccY();
            g_robotState.imuAccZ = imu.getAccZ();
            g_robotState.imuGyroZ = imu.getGyroZ();
            g_robotState.velocityL = angleL; 
            g_robotState.velocityR = angleR;
        }

        // State machine logic for the 3.5V step velocity diagnostic routine
        uint32_t currentTimeMs = millis();
        uint32_t cycleTime = (currentTimeMs / 3000) % 3; // 3-second intervals
        
        float targetTestSpeed = 0.0f;
        if (cycleTime == 0) {
            targetTestSpeed = 0.8f;  // Move forward at 80% of configured 3.5V ceiling (2.8V)
        } else if (cycleTime == 1) {
            targetTestSpeed = 0.0f;  // Rest / Standstill
        } else if (cycleTime == 2) {
            targetTestSpeed = -0.8f; // Move backward at 80% of configured 3.5V ceiling (-2.8V)
        }

        motorLeft.setSpeed(targetTestSpeed);
        motorRight.setSpeed(targetTestSpeed); // Same sign for uniform axial motion test

        // Strict periodic delay enforcement to maintain control-loop determinism
        vTaskDelayUntil(&xLastWakeTime, CONTROL_LOOP_PERIOD_MS);
    }
}

void startControlTask() {
    xTaskCreatePinnedToCore(
        controlTaskLoop,
        "ControlTask",
        4096,
        nullptr,
        3,     // High priority level
        nullptr,
        1      // Pinned strictly to Core 1
    );
}
