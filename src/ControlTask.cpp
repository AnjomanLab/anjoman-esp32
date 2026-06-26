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
extern void systemLog(const char* format, ...);

enum TestPhase {
    IDLE = 0,
    LEFT_WHEEL,
    COAST1,
    RIGHT_WHEEL,
    COAST2,
    SWEEP_MOTOR,
    TEST_COMPLETE
};

void controlTaskLoop(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    uint32_t cachedFreqL = 2000, cachedFreqR = 2000, cachedFreqSweep = 7000;
    uint8_t cachedResL = 10, cachedResR = 10, cachedResSweep = 10;
    
    uint32_t pulseEndMillis = 0;
    bool isPulsing = false;

    // Sequence state machine variables
    TestPhase activePhase = IDLE;
    uint32_t phaseEndMillis = 0;

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

        // 1. Dynamic Frequency Reconfigurations
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

        // 2. Automated Diagnostic Sequence State Machine
        if (localParams.executeSequence && activePhase == IDLE) {
            activePhase = LEFT_WHEEL;
            phaseEndMillis = millis() + 2000; // Phase 1: Left Motor 2000ms
            
            systemLog("[AUTO_TEST] Starting sequence. Phase: LEFT_WHEEL (2000ms)\n");
            
            motorLeft.setSpeed(1.0f * MOTOR_L_SIGN);
            motorRight.setSpeed(0.0f);
            motorSweep.setSpeed(0.0f);
        }

        if (activePhase != IDLE) {
            if (millis() >= phaseEndMillis) {
                // Phase transitions
                if (activePhase == LEFT_WHEEL) {
                    activePhase = COAST1;
                    phaseEndMillis = millis() + 1000; // Rest 1000ms
                    systemLog("[AUTO_TEST] Phase: COAST1 (1000ms)\n");
                    motorLeft.brake(); motorRight.brake(); motorSweep.brake();
                } 
                else if (activePhase == COAST1) {
                    activePhase = RIGHT_WHEEL;
                    phaseEndMillis = millis() + 2000; // Phase 3: Right Motor 2000ms
                    systemLog("[AUTO_TEST] Phase: RIGHT_WHEEL (2000ms)\n");
                    motorLeft.setSpeed(0.0f);
                    motorRight.setSpeed(1.0f * MOTOR_R_SIGN);
                    motorSweep.setSpeed(0.0f);
                } 
                else if (activePhase == RIGHT_WHEEL) {
                    activePhase = COAST2;
                    phaseEndMillis = millis() + 1000; // Rest 1000ms
                    systemLog("[AUTO_TEST] Phase: COAST2 (1000ms)\n");
                    motorLeft.brake(); motorRight.brake(); motorSweep.brake();
                } 
                else if (activePhase == COAST2) {
                    activePhase = SWEEP_MOTOR;
                    phaseEndMillis = millis() + 3000; // Phase 5: Sweep Motor 3000ms (Clockwise with configured sign) [1]
                    systemLog("[AUTO_TEST] Phase: SWEEP_MOTOR (3000ms)\n");
                    motorLeft.setSpeed(0.0f);
                    motorRight.setSpeed(0.0f);
                    motorSweep.setSpeed(1.0f * MOTOR_SWEEP_SIGN); // Leverages verified polarity [1]
                } 
                else if (activePhase == SWEEP_MOTOR) {
                    activePhase = IDLE;
                    motorLeft.brake(); motorRight.brake(); motorSweep.brake();
                    systemLog("[AUTO_TEST] Sequence complete. System IDLE.\n");
                    {
                        std::lock_guard<std::mutex> lock(g_stateMutex);
                        g_tuningParams.executeSequence = false;
                    }
                }
            }
        }

        // 3. Pulse / Manual Testing Fallbacks (Disabled if Sequence is running)
        if (activePhase == IDLE) {
            if (localParams.executePulse && !isPulsing) {
                isPulsing = true;
                pulseEndMillis = millis() + localParams.pulseDurationMs;
                
                uint32_t activationTimestampUs = micros();
                {
                    std::lock_guard<std::mutex> lock(g_stateMutex);
                    g_tuningParams.commandExecutionTimestampMicros = activationTimestampUs;
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
                    g_tuningParams.executePulse = false; 
                }
            }

            if (!isPulsing && !localParams.executePulse) {
                motorLeft.setSpeed(localParams.dutyL * MOTOR_L_SIGN);
                motorRight.setSpeed(localParams.dutyR * MOTOR_R_SIGN);
                motorSweep.setSpeed(localParams.dutySweep * MOTOR_SWEEP_SIGN);
            }
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
