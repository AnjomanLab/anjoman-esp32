#include <Arduino.h>
#include <Wire.h>
#include "PinMap.h"
#include "SystemConfig.h"
#include "SharedState.h"
#include "MotorController.h"
#include "MagneticEncoder.h"
#include "BMI160_Custom.h"
#include "ControlTask.h"
#include "TelemetryTask.h"
#include <Adafruit_VL53L7CX.h>

MotorController motorLeft(PIN_MOTOR_L_IN1, PIN_MOTOR_L_IN2, 0, 1);
MotorController motorRight(PIN_MOTOR_R_IN1, PIN_MOTOR_R_IN2, 2, 3);
MotorController motorSweep(PIN_SWEEP_IN1, PIN_SWEEP_IN2, 4, 5);

MagneticEncoder encoderLeft(Wire1, MUX_I2C_ADDR, MUX_CHAN_LEFT_ENCODER);
MagneticEncoder encoderRight(Wire1, MUX_I2C_ADDR, MUX_CHAN_RIGHT_ENCODER);
MagneticEncoder encoderSweep(Wire1, MUX_I2C_ADDR, MUX_CHAN_SWEEP_ENCODER);

BMI160_Custom imu(Wire1, IMU_I2C_ADDR);

void systemLog(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.print(buffer);
    Serial0.print(buffer);
}

void setup() {
    // -------------------------------------------------------------------------
    // CRITICAL FIRST LINE DEFENSE: Wake up ToF Regulator & Kill Motor Floating
    // -------------------------------------------------------------------------
    pinMode(PIN_TOF_LPN, OUTPUT);
    digitalWrite(PIN_TOF_LPN, HIGH); // Pull Wakeup pin HIGH to boot up the ToF [1]
    delay(150); // Allow LDO voltage rail to stabilize before any I2C write [1]

    pinMode(PIN_MOTOR_L_IN1, OUTPUT);
    pinMode(PIN_MOTOR_L_IN2, OUTPUT);
    pinMode(PIN_MOTOR_R_IN1, OUTPUT);
    pinMode(PIN_MOTOR_R_IN2, OUTPUT);
    pinMode(PIN_SWEEP_IN1, OUTPUT);
    pinMode(PIN_SWEEP_IN2, OUTPUT);

    digitalWrite(PIN_MOTOR_L_IN1, LOW);
    digitalWrite(PIN_MOTOR_L_IN2, LOW);
    digitalWrite(PIN_MOTOR_R_IN1, LOW);
    digitalWrite(PIN_MOTOR_R_IN2, LOW);
    digitalWrite(PIN_SWEEP_IN1, LOW);
    digitalWrite(PIN_SWEEP_IN2, LOW);

    Serial.begin(115200);   
    Serial0.begin(115200);  
    
    delay(2000); 
    systemLog("\n==================================================\n");
    systemLog("[ANJOMAN BRINGUP] Dynamic Testing Suite Initiated.\n");
    systemLog("==================================================\n");

    // Initialize Wire1 at 400kHz Fast-mode
    Wire1.begin(PIN_I2C1_SDA, PIN_I2C1_SCL, 400000U);
    Wire1.setTimeOut(50); 

    // Mount I2C0 dedicated Bus for ToF at 400kHz
    Wire.begin(PIN_I2C0_SDA, PIN_I2C0_SCL, 400000U);
    Wire.setTimeOut(50);

    // Actuator Driver default registers allocation (5kHz frequency, 10-bit resolution)
    motorLeft.begin(5000, 10, MOTOR_DUTY_LIMIT);
    motorRight.begin(5000, 10, MOTOR_DUTY_LIMIT);
    motorSweep.begin(5000, 10, MOTOR_DUTY_LIMIT);

    // Encoder initializations
    encoderLeft.begin();
    encoderRight.begin();
    encoderSweep.begin();

    // Inertial Measurement Unit initialization
    if (imu.begin()) {
        imu.configureDefault();
        systemLog("[SYSTEM] Bosch BMI160 Online.\n");
    } else {
        systemLog("[WARNING] BMI160 Offline. Bypass.\n");
    }

    startTelemetryTask(); // Port 80 Web Console + ToF Handler (Core 0)
    startControlTask();   // High frequency Realtime Motor Loop (Core 1)

    systemLog("[SYSTEM] Setup completed successfully. IP: 192.168.1.150\n");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
