#include "SystemConfig.h"

#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <Arduino.h>

// =============================================================================
// ANJOMAN HARDWARE POLARITY & SYSTEM CONFIGURATIONS (POST-CALIBRATION)
// =============================================================================

// --- Motor Output Polarity Coefficients ---
const float MOTOR_L_SIGN     = 1.0f;
const float MOTOR_R_SIGN     = 1.0f; 
const float MOTOR_SWEEP_SIGN = -1.0f; // -1.0f commands clockwise Sweep and positive encoder increase [1]

// --- Encoder Input Polarity Coefficients ---
const float ENCODER_L_SIGN     = -1.0f; // Fixed: Set to -1.0f to invert CCW forward motion to positive count [1]
const float ENCODER_R_SIGN     = 1.0f;  // Kept at 1.0f as CCW backward motion correctly counts negative [1]
const float ENCODER_SWEEP_SIGN = 1.0f;

// --- Dynamic Power Grid Constraints (Updated) ---
const float BATTERY_VOLTAGE_MAX    = 7.2f;   // Updated maximum battery voltage [1]
const float MOTOR_MAX_VOLTAGE      = 4.0f;   // Updated maximum motor allowed voltage [1]
const float MOTOR_DUTY_LIMIT       = MOTOR_MAX_VOLTAGE / BATTERY_VOLTAGE_MAX; // Calculates ceiling (~0.5555)

// --- Robot Kinematics Configurations ---
const float WHEEL_DIAMETER_METERS   = 0.050f;   // Dynamic 5.0 cm wheel diameter
const float WHEEL_RADIUS_METERS     = WHEEL_DIAMETER_METERS / 2.0f;
const float AXLE_TRACK_METERS       = 0.135f;   

// --- Loop Rates ---
const TickType_t CONTROL_LOOP_PERIOD_MS = pdMS_TO_TICKS(10);  
const TickType_t TELEMETRY_PERIOD_MS    = pdMS_TO_TICKS(100); 

// --- Multiplexer and I2C Mapping ---
const uint8_t MUX_I2C_ADDR              = 0x70; 
const uint8_t MUX_CHAN_LEFT_ENCODER     = 1;  // Left is physically Channel 1 (Swapped) [1]
const uint8_t MUX_CHAN_RIGHT_ENCODER    = 0;  // Right is physically Channel 0 (Swapped) [1]
const uint8_t MUX_CHAN_SWEEP_ENCODER    = 2;
const uint8_t IMU_I2C_ADDR              = 0x69; 

#endif // SYSTEM_CONFIG_H
