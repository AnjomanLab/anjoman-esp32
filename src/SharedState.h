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
    float imuAccX;
    float imuAccY;
    float imuAccZ;
    float imuGyroZ;
};

// Global shared variables accessed by Core 0 and Core 1
extern RobotState g_robotState;
extern std::mutex g_stateMutex;

#endif // SHARED_STATE_H
