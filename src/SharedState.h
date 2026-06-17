#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <mutex>

// Struct representing the internal states of the robot agent
struct RobotState {
    float posX;
    float posY;
    float theta;
    float velocityL;
    float velocityR;
    float velocitySweep; // Sweep Motor AS5600 Encoder angle representation
    float imuAccX;
    float imuAccY;
    float imuAccZ;
    float imuGyroZ;
    
    // Multi-zone Time of Flight matrix representations (4x4 zone layout)
    int16_t tofDistances[16]; 
};

// Global shared variables accessed by Core 0 and Core 1
extern RobotState g_robotState;
extern std::mutex g_stateMutex;

#endif // SHARED_STATE_H
