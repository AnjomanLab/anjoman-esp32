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
        // Read IMU raw values safely utilizing internal BMI160 callback locks
        imu.readSensorData();
        
        // Read Encoders utilizing static I2C1 bus mutexes
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

        // Test Sweep Routine: Safe low-excitation sinusoidal output for diagnostic first run
        float testSpeed = 0.25f * sin(esp_timer_get_time() / 1000000.0f);
        motorLeft.setSpeed(testSpeed);
        motorRight.setSpeed(-testSpeed);

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
        3,     // High priority level for real-time control calculations
        nullptr,
        1      // Pinned strictly to Core 1
    );
}
