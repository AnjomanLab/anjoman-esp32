#include "TelemetryTask.h"
#include "PinMap.h"
#include "SystemConfig.h"
#include "SharedState.h"
#include <WiFi.h>
#include <WebServer.h>

extern void systemLog(const char* format, ...);

IPAddress local_IP(192, 168, 1, 150);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

// Barebone plain text HTML UI - Zero styling, ultra lightweight under 1KB
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<body>
    <h3>ANJOMAN SYSTEM BRINGUP</h3>
    <pre id="mon">LOAD...</pre>
    <hr>
    <h4>MOTOR REGISTERS</h4>
    L: Freq <input type="number" id="fl" value="5000"> Res <input type="number" id="rl" value="10"><br>
    R: Freq <input type="number" id="fr" value="5000"> Res <input type="number" id="rr" value="10"><br>
    S: Freq <input type="number" id="fs" value="5000"> Res <input type="number" id="rs" value="10"><br>
    <button onclick="setReg()">Apply Registers</button>
    <hr>
    <h4>STEP PULSE</h4>
    L-Duty (-1.0 to 1.0): <input type="text" id="dl" value="0.0"><br>
    R-Duty (-1.0 to 1.0): <input type="text" id="dr" value="0.0"><br>
    S-Duty (-1.0 to 1.0): <input type="text" id="ds" value="0.0"><br>
    Duration (ms): <input type="number" id="dur" value="1000"><br>
    <button onclick="pulse()">Fire Step Pulse</button>
    <button onclick="stop()">STOP BRAKE</button>
    <script>
        function setReg() {
            fetch('/api/config?fl='+document.getElementById('fl').value+'&rl='+document.getElementById('rl').value+'&fr='+document.getElementById('fr').value+'&rr='+document.getElementById('rr').value+'&fs='+document.getElementById('fs').value+'&rs='+document.getElementById('rs').value);
        }
        function pulse() {
            fetch('/api/pulse?dl='+document.getElementById('dl').value+'&dr='+document.getElementById('dr').value+'&ds='+document.getElementById('ds').value+'&dur='+document.getElementById('dur').value);
        }
        function stop() { fetch('/api/stop'); }
        setInterval(() => {
            fetch('/api/status').then(r=>r.json()).then(d=>{
                document.getElementById('mon').innerText = `EncL: ${d.encL.toFixed(3)} rad | EncR: ${d.encR.toFixed(3)} rad | EncS: ${d.encSweep.toFixed(3)} rad\nAcc: ${d.accX.toFixed(2)}, ${d.accY.toFixed(2)}, ${d.accZ.toFixed(2)} G | GyroZ: ${d.gyroZ.toFixed(2)} dps`;
            });
        }, 150);
    </script>
</body>
</html>
)rawliteral";

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

void handleStatus() {
    RobotState localState;
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        localState = g_robotState;
    }
    String json = "{\"encL\":" + String(localState.velocityL, 4) + 
                  ",\"encR\":" + String(localState.velocityR, 4) + 
                  ",\"encSweep\":" + String(localState.velocitySweep, 4) + 
                  ",\"accX\":" + String(localState.imuAccX, 3) + 
                  ",\"accY\":" + String(localState.imuAccY, 3) + 
                  ",\"accZ\":" + String(localState.imuAccZ, 3) + 
                  ",\"gyroZ\":" + String(localState.imuGyroZ, 3) + "}";
    server.send(200, "application/json", json);
}

void telemetryTaskLoop(void *pvParameters) {
    WiFi.mode(WIFI_STA); // Force single station mode to save RAM and RF cycles
    WiFi.disconnect(true);
    delay(100);
    WiFi.config(local_IP, gateway, subnet);
    WiFi.begin("Oochoo", "ax200ax200");
    
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    WiFi.setSleep(false); // CRITICAL: Disable WiFi power-save to drop ping times below 5ms! [1]
    systemLog("[WIFI] Dynamic low-latency mode active. IP: %s\n", WiFi.localIP().toString().c_str());

    server.on("/", handleRoot);
    server.on("/api/config", handleConfigUpdate);
    server.on("/api/pulse", handlePulse);
    server.on("/api/stop", handleStop);
    server.on("/api/status", handleStatus);
    server.begin();

    uint32_t lastLogTime = millis();

    while (true) {
        // Handle incoming HTTP Client packets (Fixed: added handleClient() loop)
        server.handleClient();

        // Safe 1Hz logging frequency to prevent serial buffer congestion
        if (millis() - lastLogTime >= 1000) {
            lastLogTime = millis();
            RobotState localState;
            TuningParams localParams;
            {
                std::lock_guard<std::mutex> lock(g_stateMutex);
                localState = g_robotState;
                localParams = g_tuningParams;
            }
            systemLog("[TELE] TS_Us:%u | L:%.2f | R:%.2f | Sweep:%.2f | Acc_Z:%.2f\n",
                      localParams.commandExecutionTimestampMicros,
                      localState.velocityL, 
                      localState.velocityR, 
                      localState.velocitySweep,
                      localState.imuAccZ);
        }

        vTaskDelay(pdMS_TO_TICKS(5)); // Run loop at 200Hz for responsive web operations
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
