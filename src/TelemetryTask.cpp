#include "TelemetryTask.h"
#include "SystemConfig.h"
#include "SharedState.h"

// Access the dual-interface safe logging utility from main.cpp
extern void systemLog(const char* format, ...);

void telemetryTaskLoop(void *pvParameters) {
    while (true) {
        RobotState localStateCopy;
        
        // Critical Section: Thread-safe atomic capture of the current robot state
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            localStateCopy = g_robotState;
        }

        // Direct formatted stream to both Native USB CDC and CP2102 Hardware UART
        systemLog("[TELEMETRY] AccelXYZ: [%.2f, %.2f, %.2f] | GyroZ: %.2f | Enc_L: %.2f rad | Enc_R: %.2f rad\n",
                  localStateCopy.imuAccX, 
                  localStateCopy.imuAccY, 
                  localStateCopy.imuAccZ, 
                  localStateCopy.imuGyroZ,
                  localStateCopy.velocityL, 
                  localStateCopy.velocityR);

        // Standard non-deterministic loop delay for low-priority telemetry tasks
        vTaskDelay(TELEMETRY_PERIOD_MS);
    }
}

void startTelemetryTask() {
    xTaskCreatePinnedToCore(
        telemetryTaskLoop,
        "TelemetryTask",
        4096,
        nullptr,
        1,     // Low priority (allows critical control tasks to preempt)
        nullptr,
        0      // Executed on Core 0
    );
}
