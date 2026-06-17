#include "TelemetryTask.h"
#include "SystemConfig.h"
#include "SharedState.h"
#include <WiFi.h>

extern void systemLog(const char* format, ...);

// Instantiate static IP configuration for the swarm node
IPAddress local_IP(192, 168, 1, 150);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Initialize TCP Server on port 8080
WiFiServer server(8080);
WiFiClient activeClient;

void telemetryTaskLoop(void *pvParameters) {
    // 1. Initialize WiFi Configuration
    WiFi.disconnect(true);
    delay(100);

    if (!WiFi.config(local_IP, gateway, subnet)) {
        systemLog("[ERROR] Failed to configure WiFi Static IP!\n");
    }

    systemLog("[WIFI] Connecting to SSID: Oochoo ...\n");
    WiFi.begin("Oochoo", "ax200ax200");

    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    systemLog("[WIFI] Connected! Assigned IP: %s\n", WiFi.localIP().toString().c_str());

    // 2. Start TCP Server
    server.begin();
    systemLog("[TCP] Telemetry socket server active on port 8080.\n");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t x1HzPeriod = pdMS_TO_TICKS(1000); // 1Hz telemetry frequency as requested [1]

    while (true) {
        // Handle incoming or dropped TCP connections
        if (!activeClient || !activeClient.connected()) {
            activeClient = server.available();
            if (activeClient) {
                systemLog("[TCP] Client successfully connected from: %s\n", activeClient.remoteIP().toString().c_str());
            }
        }

        RobotState localStateCopy;
        
        // Critical Section: atomic retrieve of robot physical values
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            localStateCopy = g_robotState;
        }

        // Print to the local terminal (as requested)
        systemLog("[TELEMETRY] AccelXYZ: [%.2f, %.2f, %.2f] | GyroZ: %.2f | Enc_L: %.2f rad | Enc_R: %.2f rad\n",
                  localStateCopy.imuAccX, 
                  localStateCopy.imuAccY, 
                  localStateCopy.imuAccZ, 
                  localStateCopy.imuGyroZ,
                  localStateCopy.velocityL, 
                  localStateCopy.velocityR);

        // Stream raw data packets to active socket connection (nc 192.168.1.150 8080)
        if (activeClient && activeClient.connected()) {
            activeClient.printf("[TCP_TELEMETRY] Accel: [%.2f, %.2f, %.2f] | Gyro_Z: %.2f | Enc_L: %.2f | Enc_R: %.2f\n",
                                localStateCopy.imuAccX, 
                                localStateCopy.imuAccY, 
                                localStateCopy.imuAccZ, 
                                localStateCopy.imuGyroZ,
                                localStateCopy.velocityL, 
                                localStateCopy.velocityR);
        }

        // Delay loop execution to exactly 1Hz [1]
        vTaskDelayUntil(&xLastWakeTime, x1HzPeriod);
    }
}

void startTelemetryTask() {
    xTaskCreatePinnedToCore(
        telemetryTaskLoop,
        "TelemetryTask",
        4096,
        nullptr,
        1,     
        nullptr,
        0      // Pinned strictly to Core 0 (WiFi/Networking/SD Task handler)
    );
}
