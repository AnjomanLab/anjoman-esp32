#include "SharedState.h"

RobotState g_robotState = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, {0}};

TuningParams g_tuningParams = {
    0.0f, 0.0f, 0.0f,    
    5000, 5000, 5000,    
    10, 10, 10,          
    1000,                
    false,               
    0,
    8,                   // Default boot resolution is 8x8 (64 Zones)
    false,
    false                // Default self-occlusion logging is inactive
};

std::mutex g_stateMutex;
