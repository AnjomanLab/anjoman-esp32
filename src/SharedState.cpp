#include "SharedState.h"

RobotState g_robotState = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

// Default safe settings initialized at startup
TuningParams g_tuningParams = {
    0.0f, 0.0f, 0.0f,    // Default Duty cycles
    5000, 5000, 5000,    // Default Frequencies (5kHz)
    10, 10, 10,          // Default Bit Resolutions (10-bit)
    1000,                // Default Pulse Duration (1 sec)
    false,               // Standby
    0
};

std::mutex g_stateMutex;
