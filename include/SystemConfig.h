#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <Arduino.h>

// =============================================================================
// SWARM SYSTEM PARAMETERS & CONFIGURATION DEFINITIONS
// =============================================================================

// --- Power and Voltage Protection Limits ---
const float BATTERY_VOLTAGE_MAX    = 8.4f;   
const float MOTOR_MAX_VOLTAGE      = 3.5f;   
const float MOTOR_DUTY_LIMIT       = MOTOR_MAX_VOLTAGE / BATTERY_VOLTAGE_MAX; 

// --- Robot Kinematics Configurations ---
const float WHEEL_DIAMETER_METERS   = 0.050f;   // Updated wheel diameter to 5.0 cm (0.050m) as requested
const float WHEEL_RADIUS_METERS     = WHEEL_DIAMETER_METERS / 2.0f;
const float AXLE_TRACK_METERS       = 0.145f;   
const float ENCODER_TICKS_PER_REV   = 4096.0f;  

// --- Real-time Loop Rates & Core Pinning Configurations (FreeRTOS) ---
const TickType_t CONTROL_LOOP_PERIOD_MS = pdMS_TO_TICKS(10);  
const TickType_t SENSOR_READ_PERIOD_MS  = pdMS_TO_TICKS(20);  
const TickType_t TELEMETRY_PERIOD_MS    = pdMS_TO_TICKS(1000); // Set periodic terminal logging rate to 1Hz (1000ms)

// --- TCA9548A I2C Channel Assignments ---
const uint8_t MUX_I2C_ADDR              = 0x70; 
const uint8_t MUX_CHAN_LEFT_ENCODER     = 0;
const uint8_t MUX_CHAN_RIGHT_ENCODER    = 1;
const uint8_t MUX_CHAN_SWEEP_ENCODER    = 2;

// --- IMU Native Configuration Constraints ---
const uint8_t IMU_I2C_ADDR              = 0x69; 

// --- PID Speed Controller Parameters ---
const float PID_KP                      = 2.45f;
const float PID_KI                      = 0.12f;
const float PID_KD                      = 0.05f;

#endif // SYSTEM_CONFIG_H
