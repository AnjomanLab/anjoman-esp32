#include "TelemetryTask.h"
#include "SystemConfig.h"
#include "SharedState.h"

void telemetryTaskLoop(void *pvParameters) {
    while (true) {
        RobotState localStateCopy;
        
        // Critical Section: Copy state with minimal hold time to prevent blocking Core 1
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            localStateCopy = g_robotState;
        }

        // Dump data over configured Native USB CDC Interface
        Serial.printf("[TELEMETRY] AccelXYZ: [%.2f, %.2f, %.2f] | GyroZ: %.2f | Enc_L: %.2f rad | Enc_R: %.2f rad\n",
                      localStateCopy.imuAccX, 
                      localStateCopy.imuAccY, 
                      localStateCopy.imuAccZ, 
                      localStateCopy.imuGyroZ,
                      localStateCopy.velocityL, 
                      localStateCopy.velocityR);

        // Standard non-deterministic delay for background tasks
        vTaskDelay(TELEMETRY_PERIOD_MS);
    }
}

void startTelemetryTask() {
    xTaskCreatePinnedToCore(
        telemetryTaskLoop,
        "TelemetryTask",
        4096,
        nullptr,
        1,     // Lower priority to allow network stacks to process overhead smoothly
        nullptr,
        0      // Pinned strictly to Core 0
    );
}
