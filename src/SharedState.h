#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <mutex>
#include <Arduino.h>

// Realtime state parameters updated from Core 1
struct RobotState {
    float velocityL;
    float velocityR;
    float velocitySweep;
    float imuAccX;
    float imuAccY;
    float imuAccZ;
    float imuGyroZ;
};

// Interactive parameters updated from Web Server on Core 0
struct TuningParams {
    float dutyL;
    float dutyR;
    float dutySweep;
    
    uint32_t freqL;
    uint32_t freqR;
    uint32_t freqSweep;
    
    uint8_t resL;
    uint8_t resR;
    uint8_t resSweep;
    
    uint32_t pulseDurationMs;
    bool executePulse;
    
    uint32_t commandExecutionTimestampMicros; // Precision timing for telemetry speed math
};

extern RobotState g_robotState;
extern TuningParams g_tuningParams;
extern std::mutex g_stateMutex;

#endif // SHARED_STATE_H
