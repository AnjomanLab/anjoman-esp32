#include "TelemetryTask.h"
#include "PinMap.h"
#include "SystemConfig.h"
#include "SharedState.h"
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VL53L7CX.h>

extern void systemLog(const char* format, ...);
Adafruit_VL53L7CX vl53; 

IPAddress local_IP(192, 168, 1, 150);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

File logFile;
char logFileName[32];
bool sdCardMounted = false;

// Barebone plain text HTML console with dynamic status monitors & absolute parameter Wiki guide
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<body>
    <h3>ANJOMAN SYSTEM DIAGNOSTIC PANEL</h3>
    <pre id="mon">RETRIEVING STATUS DATAPACKETS...</pre>
    <hr>
    <h4>ACTUATOR REGISTERS CONFIGURATION</h4>
    Left Motor:  Freq <input type="number" id="fl" value="2000"> Hz | Res <input type="number" id="rl" value="10"> bits<br>
    Right Motor: Freq <input type="number" id="fr" value="2000"> Hz | Res <input type="number" id="rr" value="10"> bits<br>
    Sweep Motor: Freq <input type="number" id="fs" value="7000"> Hz | Res <input type="number" id="rs" value="10"> bits<br>
    <button onclick="setReg()">Write Actuator Registers</button>
    <hr>
    <h4>STEP RESPONSE TEST PANEL</h4>
    Left Motor Duty (-1.0 to 1.0):  <input type="text" id="dl" value="0.0"><br>
    Right Motor Duty (-1.0 to 1.0): <input type="text" id="dr" value="0.0"><br>
    Sweep Motor Duty (-1.0 to 1.0): <input type="text" id="ds" value="0.0"><br>
    Pulse Step Duration (ms):       <input type="number" id="dur" value="1000"><br>
    <button onclick="pulse()">Fire Bounded Step Pulse</button>
    <button style="background:red; color:white;" onclick="stop()">STOP HARD BRAKE</button>
    <hr>
    <h4>SYSTEMATIC DIAGNOSTIC SUITE</h4>
    <button style="background:green; color:black; font-weight:bold; height:35px;" onclick="runSequence()">RUN AUTOMATED SEQUENCE TEST (9000ms)</button>
    <hr>
    <h4>SENSING & LOCALIZATION</h4>
    ToF Ranging Resolution Matrix: 
    <select id="tof_res">
        <option value="4">4x4 Matrix (16 Zones)</option>
        <option value="8" selected>8x8 Matrix (64 Zones)</option>
    </select>
    <button onclick="setTofRes()">Apply Matrix Resolution</button>
    <br><br>
    Self-Occlusion Logging Mode (Serial Filter): 
    <input type="checkbox" id="occlusion" onchange="toggleOcclusion(this.checked)"> Active
    <hr>
    
    <h4>WIKI GUIDE & DESCRIPTION OF PARAMETERS</h4>
    <pre>
-----------------------------------------------------------------------------------------
PARAMETER             | TYPE    | BEHAVIOR & PHYSICAL REACTION IN BRINGUP PHASE
-----------------------------------------------------------------------------------------
Left/Right Motor Duty | float   | Maps directly to driver voltage. 1.0 is full forward,
                      |         | -1.0 is full reverse. Used to match wheel directions.
Sweep Motor Duty      | float   | Regulates ToF rotation. Controls torque and speed.
Freq (Hz)             | integer | Swaps timer frequency on S3 LEDC registers. Lowering
                      |         | frequency (e.g., 2000Hz) dramatically increases torque.
Res (Bits)            | integer | Sets resolution bit width (8, 10, or 12). 10 bits maps
                      |         | duty cycles from 0 to 1023 steps.
Pulse Duration (ms)   | integer | Bounded time for motor actuation. Halts and brakes 
                      |         | immediately when the epoch ends.
ToF Resolution        | enum    | 4x4 provides 16 distance zones up to 30Hz. 
                      |         | 8x8 provides high density 64 zones up to 15Hz.
Self-Occlusion Mode   | boolean | Suspends all telemetry logs in serial except: 
                      |         | [ToF Sweep Angle] and [ToF Center Distance]. 
                      |         | Essential to capture the precise coordinate of the nose.
-----------------------------------------------------------------------------------------
    </pre>

    <script>
        function setReg() {
            fetch('/api/config?fl='+document.getElementById('fl').value+'&rl='+document.getElementById('rl').value+'&fr='+document.getElementById('fr').value+'&rr='+document.getElementById('rr').value+'&fs='+document.getElementById('fs').value+'&rs='+document.getElementById('rs').value);
        }
        function pulse() {
            fetch('/api/pulse?dl='+document.getElementById('dl').value+'&dr='+document.getElementById('dr').value+'&ds='+document.getElementById('ds').value+'&dur='+document.getElementById('dur').value);
        }
        function stop() { fetch('/api/stop'); }
        
        function runSequence() {
            fetch('/api/sequence').then(r => r.text()).then(t => alert(t));
        }

        function setTofRes() {
            fetch('/api/tof?res=' + document.getElementById('tof_res').value);
        }
        function toggleOcclusion(val) {
            fetch('/api/occlusion?val=' + (val ? "1" : "0"));
        }

        setInterval(() => {
            fetch('/api/status').then(r=>r.json()).then(d=>{
                document.getElementById('mon').innerText = `EncL: ${d.encL.toFixed(3)} rad | EncR: ${d.encR.toFixed(3)} rad | EncS: ${d.encSweep.toFixed(3)} rad\nAcc: ${d.accX.toFixed(2)}, ${d.accY.toFixed(2)}, ${d.accZ.toFixed(2)} G | GyroZ: ${d.gyroZ.toFixed(2)} dps\nToF Center Distance: ${d.tofCenter} mm`;
            });
        }, 150);
    </script>
</body>
</html>
)rawliteral";

void initializeSD() {
    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SD_CS);
    if (!SD.begin(PIN_SD_CS, SPI, 4000000U)) {
        systemLog("[WARNING] Micro SD Card Mount Failed. Continuing without physical logger.\n");
        return;
    }
    
    snprintf(logFileName, sizeof(logFileName), "/tof_8x8_%u.csv", millis());
    logFile = SD.open(logFileName, FILE_WRITE);
    if (logFile) {
        logFile.print("Time_ms,Enc_L,Enc_R,Enc_Sweep_Rad,Acc_X,Acc_Y,Gyro_Z");
        for(int i = 0; i < 64; i++) {
            logFile.printf(",Z%d", i);
        }
        logFile.println();
        logFile.close();
        sdCardMounted = true;
        systemLog("[LOGGER] Micro SD Logging active: %s\n", logFileName);
    }
}

void handleRoot() {
    server.send_P(200, "text/html", INDEX_HTML);
}

void handleConfigUpdate() {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    if (server.hasArg("fl")) g_tuningParams.freqL = server.arg("fl").toInt();
    if (server.hasArg("rl")) g_tuningParams.resL = server.arg("rl").toInt();
    if (server.hasArg("fr")) g_tuningParams.freqR = server.arg("fr").toInt();
    if (server.hasArg("rr")) g_tuningParams.resR = server.arg("rr").toInt();
    if (server.hasArg("fs")) g_tuningParams.freqSweep = server.arg("fs").toInt();
    if (server.hasArg("rs")) g_tuningParams.resSweep = server.arg("rs").toInt();
    server.send(200, "text/plain", "OK");
}

void handlePulse() {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    if (server.hasArg("dl")) g_tuningParams.dutyL = server.arg("dl").toFloat();
    if (server.hasArg("dr")) g_tuningParams.dutyR = server.arg("dr").toFloat();
    if (server.hasArg("ds")) g_tuningParams.dutySweep = server.arg("ds").toFloat();
    if (server.hasArg("dur")) g_tuningParams.pulseDurationMs = server.arg("dur").toInt();
    g_tuningParams.executePulse = true;
    server.send(200, "text/plain", "OK");
}

void handleStop() {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    g_tuningParams.dutyL = 0.0f;
    g_tuningParams.dutyR = 0.0f;
    g_tuningParams.dutySweep = 0.0f;
    g_tuningParams.executePulse = false;
    server.send(200, "text/plain", "OK");
}

void handleSequenceRun() {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    g_tuningParams.executeSequence = true; // Set state machine trigger flag [1]
    server.send(200, "text/plain", "Automated Profiling Sequence Started.");
}

void handleToFResolution() {
    if (server.hasArg("res")) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        g_tuningParams.targetToFResolution = server.arg("res").toInt();
        g_tuningParams.triggerResolutionChange = true; 
    }
    server.send(200, "text/plain", "OK");
}

void handleOcclusionToggle() {
    if (server.hasArg("val")) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        g_tuningParams.selfOcclusionMode = (server.arg("val") == "1");
    }
    server.send(200, "text/plain", "OK");
}

void handleStatus() {
    RobotState localState;
    TuningParams localParams;
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        localState = g_robotState;
        localParams = g_tuningParams;
    }
    int16_t centerDistance = (localParams.targetToFResolution == 8) ? localState.tofDistances[27] : localState.tofDistances[6];

    String json = "{\"encL\":" + String(localState.velocityL, 4) + 
                  ",\"encR\":" + String(localState.velocityR, 4) + 
                  ",\"encSweep\":" + String(localState.velocitySweep, 3) + 
                  ",\"accX\":" + String(localState.imuAccX, 3) + 
                  ",\"accY\":" + String(localState.imuAccY, 3) + 
                  ",\"accZ\":" + String(localState.imuAccZ, 3) + 
                  ",\"gyroZ\":" + String(localState.imuGyroZ, 3) + 
                  ",\"tofCenter\":" + String(centerDistance) + "}";
    server.send(200, "application/json", json);
}

void telemetryTaskLoop(void *pvParameters) {
    initializeSD();

    WiFi.mode(WIFI_STA); 
    WiFi.disconnect(true);
    delay(100);
    WiFi.config(local_IP, gateway, subnet);
    WiFi.begin("Oochoo", "ax200ax200");
    
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    WiFi.setSleep(false); 
    systemLog("[WIFI] Dynamic low-latency mode active. IP: %s\n", WiFi.localIP().toString().c_str());

    // Initialize ToF in default 8x8 mode
    systemLog("[TOF] Mounting VL53L7CX firmware over I2C0...\n");
    if (vl53.begin()) {
        vl53.setResolution(VL53L7CX_RESOLUTION_8X8);
        vl53.setRangingFrequency(15);
        vl53.startRanging();
        systemLog("[TOF] VL53L7CX online.\n");
    }

    server.on("/", handleRoot);
    server.on("/api/config", handleConfigUpdate);
    server.on("/api/pulse", handlePulse);
    server.on("/api/stop", handleStop);
    server.on("/api/status", handleStatus);
    server.on("/api/sequence", handleSequenceRun);
    server.on("/api/tof", handleToFResolution);
    server.on("/api/occlusion", handleOcclusionToggle);
    server.begin();

    uint32_t lastLogTime = millis();

    while (true) {
        server.handleClient();

        bool resChangeTriggered = false;
        uint8_t nextRes = 8;
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            resChangeTriggered = g_tuningParams.triggerResolutionChange;
            nextRes = g_tuningParams.targetToFResolution;
        }

        if (resChangeTriggered) {
            systemLog("[TOF] Reconfiguring resolution to %dx%d...\n", nextRes, nextRes);
            vl53.stopRanging();
            if (nextRes == 8) {
                vl53.setResolution(VL53L7CX_RESOLUTION_8X8);
                vl53.setRangingFrequency(15);
            } else {
                vl53.setResolution(VL53L7CX_RESOLUTION_4X4);
                vl53.setRangingFrequency(30);
            }
            vl53.startRanging();
            {
                std::lock_guard<std::mutex> lock(g_stateMutex);
                g_tuningParams.triggerResolutionChange = false;
            }
            systemLog("[TOF] Resolution reconfigured.\n");
        }

        int16_t tempDistances[64] = {0};
        bool isNewFrameCaptured = false;
        if (vl53.isDataReady()) {
            VL53L7CX_ResultsData results;
            if (vl53.getRangingData(&results)) {
                isNewFrameCaptured = true;
                for (int i = 0; i < (nextRes * nextRes); i++) {
                    tempDistances[i] = results.distance_mm[i];
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            for (int i = 0; i < 64; i++) {
                g_robotState.tofDistances[i] = tempDistances[i];
            }
        }

        RobotState localState;
        TuningParams localParams;
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            localState = g_robotState;
            localParams = g_tuningParams;
        }

        // Write telemetry data in raw cumulative radians [1]
        if (isNewFrameCaptured && sdCardMounted) {
            logFile = SD.open(logFileName, FILE_APPEND);
            if (logFile) {
                logFile.printf("%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f", 
                               millis(), localState.velocityL, localState.velocityR, localState.velocitySweep,
                               localState.imuAccX, localState.imuAccY, localState.imuGyroZ);
                for (int i = 0; i < (nextRes * nextRes); i++) {
                    logFile.printf(",%d", localState.tofDistances[i]);
                }
                logFile.println();
                logFile.close();
                
                systemLog("[SD_WRITE] Frame stored. Sweep Angle: %.3f rad\n", localState.velocitySweep);
            }
        }

        if (millis() - lastLogTime >= 100) { 
            lastLogTime = millis();
            
            if (!localParams.selfOcclusionMode) {
                systemLog("[TELE] TS_Us:%u | L:%.2f | R:%.2f | Sweep:%.3f | Acc_Z:%.2f\n",
                          localParams.commandExecutionTimestampMicros,
                          localState.velocityL, 
                          localState.velocityR, 
                          localState.velocitySweep,
                          localState.imuAccZ);
            } else {
                int16_t centerDistance = (localParams.targetToFResolution == 8) ? localState.tofDistances[27] : localState.tofDistances[6];
                systemLog("[OCCLUSION_TEST] Angle_Rad: %.3f | Center_Distance: %d mm\n",
                          localState.velocitySweep, centerDistance);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5)); 
    }
}

void startTelemetryTask() {
    xTaskCreatePinnedToCore(
        telemetryTaskLoop,
        "TelemetryTask",
        8192,  
        nullptr,
        1,     
        nullptr,
        0      
    );
}
