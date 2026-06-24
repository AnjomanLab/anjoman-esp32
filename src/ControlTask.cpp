#include "ControlTask.h"
#include "SystemConfig.h"
#include "SharedState.h"
#include "MotorController.h"
#include "MagneticEncoder.h"
#include "BMI160_Custom.h"

// External references to hardware instances
extern MotorController motorLeft;
extern MotorController motorRight;
extern MotorController motorSweep;

extern MagneticEncoder encoderLeft;
extern MagneticEncoder encoderRight;
extern MagneticEncoder encoderSweep;

extern BMI160_Custom imu;

// Declaring the global dual-interface logger
extern void systemLog(const char* format, ...);

void controlTaskLoop(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    // Cache to monitor dynamic register changes at runtime
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

        // 2. Pulse Controller Logic with embedded hardware parameter tracking [1]
        if (localParams.executePulse && !isPulsing) {
            isPulsing = true;
            pulseEndMillis = millis() + localParams.pulseDurationMs;
            
            uint32_t activationTimestampUs = micros();
            {
                std::lock_guard<std::mutex> lock(g_stateMutex);
                g_tuningParams.commandExecutionTimestampMicros = activationTimestampUs;
            }

            // High priority unified system log depicting exact input conditions at startup edge [1]
            systemLog("[PULSE_START] TS_Us:%u | L_Duty:%.2f | R_Duty:%.2f | S_Duty:%.2f | L_Freq:%u | R_Freq:%u | S_Freq:%u | L_Res:%u | R_Res:%u | S_Res:%u | Dur_Ms:%u\n",
                      activationTimestampUs,
                      localParams.dutyL, localParams.dutyR, localParams.dutySweep,
                      localParams.freqL, localParams.freqR, localParams.freqSweep,
                      localParams.resL, localParams.resR, localParams.resSweep,
                      localParams.pulseDurationMs);

            motorLeft.setSpeed(localParams.dutyL * MOTOR_L_SIGN);
            motorRight.setSpeed(localParams.dutyR * MOTOR_R_SIGN);
            motorSweep.setSpeed(localParams.dutySweep * MOTOR_SWEEP_SIGN);
        }

        // Pulse boundary termination and active electrical braking
        if (isPulsing && millis() >= pulseEndMillis) {
            motorLeft.brake();
            motorRight.brake();
            motorSweep.brake();
            isPulsing = false;
            
            // Mark the exact microsecond timestamp of the voltage termination edge [1]
            systemLog("[PULSE_END] TS_Us:%u\n", micros());
            
            {
                std::lock_guard<std::mutex> lock(g_stateMutex);
                g_tuningParams.executePulse = false; 
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
