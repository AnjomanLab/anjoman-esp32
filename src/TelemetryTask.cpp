#include "TelemetryTask.h"
#include "SystemConfig.h"
#include "SharedState.h"
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VL53L7CX.h>

extern void systemLog(const char* format, ...);

// Static IP and server structures
IPAddress local_IP(192, 168, 1, 150);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(8080);
WiFiClient activeClient;

// Hardware drivers instantiation for Core 0 exclusive sensors
Adafruit_VL53L7CX vl53;
File logFile;
char logFileName[32];

void initializeSD() {
    // Initialize standard SD Card SPI interface
    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SD_CS);
    if (!SD.begin(PIN_SD_CS, SPI, 4000000U)) {
        systemLog("[WARNING] Micro SD Card Mount Failed. Continuing without physical logger.\n");
        return;
    }
    
    // Generate unique CSV log file name based on startup epoch counter
    snprintf(logFileName, sizeof(logFileName), "/tof_run_%u.csv", millis());
    logFile = SD.open(logFileName, FILE_WRITE);
    if (logFile) {
        logFile.println("Timestamp_ms,Enc_L,Enc_R,Enc_Sweep,ToF_Zone0,ToF_Zone1,ToF_Zone2,ToF_Zone3,ToF_Zone4,ToF_Zone5,ToF_Zone6,ToF_Zone7,ToF_Zone8,ToF_Zone9,ToF_Zone10,ToF_Zone11,ToF_Zone12,ToF_Zone13,ToF_Zone14,ToF_Zone15");
        logFile.close();
        systemLog("[LOGGER] Micro SD ready! Active file: %s\n", logFileName);
    } else {
        systemLog("[WARNING] Failed to write header on Micro SD file.\n");
    }
}

void telemetryTaskLoop(void *pvParameters) {
    // 1. Initialise SD Card Logger
    initializeSD();

    // 2. Initialise ToF Sensor (Using 400kHz Fast I2C0 Bus as requested)
    systemLog("[TOF] Uploading firmware blob over I2C0... Please wait (can take up to 10 seconds)\n");
    if (vl53.begin()) { // Default maps to Wire (I2C0)
        vl53.setResolution(VL53L7CX_RESOLUTION_4X4); // Efficient 4x4 matrix representation
        vl53.setRangingFrequency(15);                 // 15Hz sensor update frequency
        vl53.startRanging();
        systemLog("[TOF] Adafruit VL53L7CX initialized successfully.\n");
    } else {
        systemLog("[CRITICAL ERROR] Failed to detect or flash Adafruit VL53L7CX ToF sensor on I2C0.\n");
    }

    // 3. Connect WiFi AP
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

    // 4. Start TCP Server
    server.begin();
    systemLog("[TCP] Telemetry socket server active on port 8080.\n");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t x1HzPeriod = pdMS_TO_TICKS(1000); // 1Hz telemetry frequency [1]

    while (true) {
        // Read ToF data asynchronously if ready
        int16_t currentDistances[16] = {0};
        if (vl53.isRanging() && vl53.checkForData()) {
            VL53L7CX_ResultsData results;
            if (vl53.getRangingData(&results)) {
                for (int i = 0; i < 16; i++) {
                    currentDistances[i] = results.distance_mm[i];
                }
            }
        }

        // Manage TCP Client connection
        if (!activeClient || !activeClient.connected()) {
            activeClient = server.available();
            if (activeClient) {
                systemLog("[TCP] Client connected: %s\n", activeClient.remoteIP().toString().c_str());
            }
        }

        RobotState localStateCopy;
        
        // Critical Section: Extract state from Core 1 and insert ToF to shared map
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            for (int i = 0; i < 16; i++) {
                g_robotState.tofDistances[i] = currentDistances[i];
            }
            localStateCopy = g_robotState;
        }

        // Print Telemetry with new 3rd Sweep Encoder to the local terminal (1Hz)
        systemLog("[TELEMETRY] AccelXYZ: [%.2f, %.2f, %.2f] | GyroZ: %.2f | Enc_L: %.2f rad | Enc_R: %.2f rad | Enc_S: %.2f rad | ToF_Ctr: %d mm\n",
                  localStateCopy.imuAccX, 
                  localStateCopy.imuAccY, 
                  localStateCopy.imuAccZ, 
                  localStateCopy.imuGyroZ,
                  localStateCopy.velocityL, 
                  localStateCopy.velocityR,
                  localStateCopy.velocitySweep,
                  localStateCopy.tofDistances[6]); // Sample the center zone 6 of 4x4 matrix

        // Stream raw data packets to active socket connection (nc 192.168.1.150 8080)
        if (activeClient && activeClient.connected()) {
            activeClient.printf("[TCP_TELEMETRY] Accel: [%.2f, %.2f, %.2f] | Gyro_Z: %.2f | Enc_L: %.2f | Enc_R: %.2f | Enc_Sweep: %.2f | ToF_Ctr: %d mm\n",
                                localStateCopy.imuAccX, 
                                localStateCopy.imuAccY, 
                                localStateCopy.imuAccZ, 
                                localStateCopy.imuGyroZ,
                                localStateCopy.velocityL, 
                                localStateCopy.velocityR,
                                localStateCopy.velocitySweep,
                                localStateCopy.tofDistances[6]);
        }

        // Write telemetry to CSV on Micro SD Card
        logFile = SD.open(logFileName, FILE_APPEND);
        if (logFile) {
            logFile.printf("%u,%.3f,%.3f,%.3f", millis(), localStateCopy.velocityL, localStateCopy.velocityR, localStateCopy.velocitySweep);
            for (int i = 0; i < 16; i++) {
                logFile.printf(",%d", localStateCopy.tofDistances[i]);
            }
            logFile.println();
            logFile.close();
        }

        vTaskDelayUntil(&xLastWakeTime, x1HzPeriod);
    }
}

void startTelemetryTask() {
    xTaskCreatePinnedToCore(
        telemetryTaskLoop,
        "TelemetryTask",
        8192,  // Extended stack size to accommodate SD file write buffer allocation
        nullptr,
        1,     
        nullptr,
        0      
    );
}
