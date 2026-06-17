# Anjoman ESP32 Swarm Robot Platform (پلتفرم رباتیک جمعی انجمن)

This repository contains the unified, single-MCU firmware for the **Anjoman** (Assembly/Swarm) research project. The platform is designed for leaderless, decentralized multi-robot formation control, powered by the ESP32-S3 microcontroller.

این مخزن شامل فریم‌ور یکپارچه تک‌تراشه‌ای برای پروژه تحقیقاتی **انجمن** است. این پلتفرم با هدف کنترل آرایش تیمی غیرمتمرکز و بدون رهبر، حول پردازنده ESP32-S3 توسعه یافته است.

---

## English Documentation

### 1. Hardware Architecture
The hardware system is designed around a single ESP32-S3-DevKitC-1 (WROOM-1-N8R2) featuring 8MB Flash and 2MB PSRAM. It utilizes an isolated power grid to prevent noise propagation:
*   **Power Supply:** 7.4V - 8.4V Li-Ion BMS main source.
*   **Grounding:** Single central node "Star Grounding" at the BMS negative terminal to prevent ground bounce.
*   **High Power (VMOT):** DRV8833 motor drivers connected directly to the BMS.
*   **5V Buck Converter:** Powers the ESP32-S3, Micro SD, VL53L7CX ToF, and DW1000 UWB.
*   **3.3V Buck Converter:** Powers the TCA9548A Multiplexer, 3x AS5600 Encoders, and BMI160 IMU.

### 2. Software & Core Affinity Layout
The firmware utilizes FreeRTOS to enforce real-time determinism across two cores:
*   **Core 0 (Networking & Storage):** Handles WiFi communication, UDP/TCP telemetry, and SD Card writing.
*   **Core 1 (Real-time Loops):** Manages MCPWM/LEDC motor execution, shared I2C1 reads (IMU/Muxed Encoders), and continuous odometry.

