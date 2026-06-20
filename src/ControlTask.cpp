#include "ControlTask.h"
#include "SystemConfig.h"
#include "SharedState.h"
#include "MotorController.h"
#include "MagneticEncoder.h"
#include "BMI160_Custom.h"

extern MotorController motorLeft;
extern MotorController motorRight;
extern MotorController motorSweep;

extern MagneticEncoder encoderLeft;
extern MagneticEncoder encoderRight;
extern MagneticEncoder encoderSweep;

extern BMI160_Custom imu;

void controlTaskLoop(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    // Dynamic tracking of runtime configuration parameters
    uint32_t cachedFreqL = 5000, cachedFreqR = 5000, cachedFreqSweep = 5000;
    uint8_t cachedResL = 10, cachedResR = 10, cachedResSweep = 10;
    
    uint32_t pulseEndMillis = 0;
    bool isPulsing = false;

    while (true) {
        imu.readSensorData();

        float angleL = encoderLeft.getAngleRadians() * ENCODER_L_SIGN;
        float angleR = encoderRight.getAngleRadians() * ENCODER_R_SIGN;
        float angleSweep = encoderSweep.getAngleRadians() * ENCODER_SWEEP_SIGN;

        TuningParams localParams;
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            g_robotState.imuAccX = imu.getAccX();
            g_robotState.imuAccY = imu.getAccY();
            g_robotState.imuAccZ = imu.getAccZ();
            g_robotState.imuGyroZ = imu.getGyroZ();
            g_robotState.velocityL = angleL;
            g_robotState.velocityR = angleR;
            g_robotState.velocitySweep = angleSweep;
            localParams = g_tuningParams;
        }

        // 1. Dynamic Frequency & Resolution updates at runtime
        if (localParams.freqL != cachedFreqL || localParams.resL != cachedResL) {
            motorLeft.reconfigure(localParams.freqL, localParams.resL);
            cachedFreqL = localParams.freqL; cachedResL = localParams.resL;
        }
        if (localParams.freqR != cachedFreqR || localParams.resR != cachedResR) {
            motorRight.reconfigure(localParams.freqR, localParams.resR);
            cachedFreqR = localParams.freqR; cachedResR = localParams.resR;
        }
        if (localParams.freqSweep != cachedFreqSweep || localParams.resSweep != cachedResSweep) {
            motorSweep.reconfigure(localParams.freqSweep, localParams.resSweep);
            cachedFreqSweep = localParams.freqSweep; cachedResSweep = localParams.resSweep;
        }

        // 2. Pulse Controller Logic (Time-bounded execution)
        if (localParams.executePulse && !isPulsing) {
            isPulsing = true;
            pulseEndMillis = millis() + localParams.pulseDurationMs;
            
            // Mark microsecond timestamp for precise velocity calculations
            {
                std::lock_guard<std::mutex> lock(g_stateMutex);
                g_tuningParams.commandExecutionTimestampMicros = micros();
            }

            motorLeft.setSpeed(localParams.dutyL * MOTOR_L_SIGN);
            motorRight.setSpeed(localParams.dutyR * MOTOR_R_SIGN);
            motorSweep.setSpeed(localParams.dutySweep * MOTOR_SWEEP_SIGN);
        }

        if (isPulsing && millis() >= pulseEndMillis) {
            motorLeft.brake();
            motorRight.brake();
            motorSweep.brake();
            isPulsing = false;
            {
                std::lock_guard<std::mutex> lock(g_stateMutex);
                g_tuningParams.executePulse = false; // Reset trigger state
            }
        }

        // 3. Continuous Drive Mode (if Execute Pulse is disabled)
        if (!isPulsing && !localParams.executePulse) {
            motorLeft.setSpeed(localParams.dutyL * MOTOR_L_SIGN);
            motorRight.setSpeed(localParams.dutyR * MOTOR_R_SIGN);
            motorSweep.setSpeed(localParams.dutySweep * MOTOR_SWEEP_SIGN);
        }

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
