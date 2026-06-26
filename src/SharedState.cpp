#include "SharedState.h"

RobotState g_robotState = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, {0}};

// Initializing the boot registers exactly to your measured stable frequencies [1]
TuningParams g_tuningParams = {
    0.0f, 0.0f, 0.0f,    
    2000, 2000, 7000,    // Left: 2kHz, Right: 2kHz, Sweep: 7kHz as measured [1]
    10, 10, 10,          // 10-bit dynamic resolution
    1000,                
    false,               
    0,
    8,                   
    false,
    false,
    false                // Default sequence run is standby
};

std::mutex g_stateMutex;
