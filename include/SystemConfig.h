#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <Arduino.h>

// --- Hardware Polarity & Mapping Correction Flags ---
const float MOTOR_L_SIGN     = 1.0f;
const float MOTOR_R_SIGN     = -1.0f; // Set to -1.0f if inverted on chassis mounting
const float MOTOR_SWEEP_SIGN = 1.0f;

const float ENCODER_L_SIGN     = 1.0f;
const float ENCODER_R_SIGN     = -1.0f; // Synced with inverted motor rotation direction
const float ENCODER_SWEEP_SIGN = 1.0f;

// --- Default Power and PWM Limits at Boot ---
const float BATTERY_VOLTAGE_MAX    = 8.4f;   
const float MOTOR_MAX_VOLTAGE      = 3.5f;   
const float MOTOR_DUTY_LIMIT       = MOTOR_MAX_VOLTAGE / BATTERY_VOLTAGE_MAX; 

// --- Robot Kinematics Configurations ---
const float WHEEL_DIAMETER_METERS   = 0.050f;   
const float WHEEL_RADIUS_METERS     = WHEEL_DIAMETER_METERS / 2.0f;
const float AXLE_TRACK_METERS       = 0.135f;   

// --- Loop Rates ---
const TickType_t CONTROL_LOOP_PERIOD_MS = pdMS_TO_TICKS(10);  
const TickType_t TELEMETRY_PERIOD_MS    = pdMS_TO_TICKS(100); // 10Hz terminal streaming

// --- I2C Addresses & Channels ---
const uint8_t MUX_I2C_ADDR              = 0x70; 
const uint8_t MUX_CHAN_LEFT_ENCODER     = 0;  
const uint8_t MUX_CHAN_RIGHT_ENCODER    = 1;
const uint8_t MUX_CHAN_SWEEP_ENCODER    = 2;
const uint8_t IMU_I2C_ADDR              = 0x69; 

#endif // SYSTEM_CONFIG_H
