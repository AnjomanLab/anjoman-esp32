#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <mutex>
#include <Arduino.h>

struct RobotState {
    float velocityL;
    float velocityR;
    float velocitySweep;
    float imuAccX;
    float imuAccY;
    float imuAccZ;
    float imuGyroZ;
    int16_t tofDistances[64]; // Stores raw 4x4 (first 16) or 8x8 matrix output
};

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
    
    uint32_t commandExecutionTimestampMicros;
    
    // Self-Occlusion and Dynamic Sensing configuration registers
    uint8_t targetToFResolution; // 4 = 4x4 Mode, 8 = 8x8 Mode
    bool triggerResolutionChange;
    bool selfOcclusionMode;      // Filters out all serial logs except Sweep Angle and Center ToF
};

extern RobotState g_robotState;
extern TuningParams g_tuningParams;
extern std::mutex g_stateMutex;

#endif // SHARED_STATE_H
