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
extern MotorController motorSweep;

extern MagneticEncoder encoderLeft;
extern MagneticEncoder encoderRight;
extern MagneticEncoder encoderSweep;

extern BMI160_Custom imu;

void controlTaskLoop(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (true) {
        // Read IMU raw values safely
        imu.readSensorData();
        
        // Read Encoders utilizing I2C1 bus mutexes
        float angleL = encoderLeft.getAngleRadians();
        float angleR = encoderRight.getAngleRadians();
        float angleSweep = encoderSweep.getAngleRadians();
        
        // Critical Section: Safely update shared global state
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            g_robotState.imuAccX = imu.getAccX();
            g_robotState.imuAccY = imu.getAccY();
            g_robotState.imuAccZ = imu.getAccZ();
            g_robotState.imuGyroZ = imu.getGyroZ();
            g_robotState.velocityL = angleL; 
            g_robotState.velocityR = angleR;
            g_robotState.velocitySweep = angleSweep; // Feed ToF sweep encoder to telemetry
        }

        // Diagnostic Step Motion: 3 seconds forward, 3 seconds idle, 3 seconds reverse
        uint32_t currentTimeMs = millis();
        uint32_t cycleTime = (currentTimeMs / 3000) % 3; 
        
        float targetTestSpeed = 0.0f;
        if (cycleTime == 0) {
            targetTestSpeed = 1.0f;  // Run at 100% of allowed 3.5V ceiling (highest safe torque)
        } else if (cycleTime == 1) {
            targetTestSpeed = 0.0f;  
        } else if (cycleTime == 2) {
            targetTestSpeed = -1.0f; 
        }

        motorLeft.setSpeed(targetTestSpeed);
        motorRight.setSpeed(targetTestSpeed);
        motorSweep.setSpeed(targetTestSpeed); // Drive Sweep motor programmatically

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
        3,     
        nullptr,
        1      
    );
}
