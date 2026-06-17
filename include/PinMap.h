#ifndef PIN_MAP_H
#define PIN_MAP_H

// =============================================================================
// ESP32-S3 DEVKITC 1 (WROOM-1-N8R2) HARDWARE PINMAP CONFIGURATION
// =============================================================================

// --- I2C0 Bus Configuration (Dedicated to ToF Multi-Zone Array) ---
// Switched to safe GPIOs on the left header, leaving UART0 (GPIO 43/44) isolated
#define PIN_I2C0_SDA            11
#define PIN_I2C0_SCL            12
#define PIN_TOF_LPN             14  // Wakeup pin (Low Power Notification)
#define PIN_TOF_INT             9   // ToF Interrupt pin (Optional but routed)

// --- I2C1 Bus Configuration (Shared by BMI160 IMU & TCA9548A Multiplexer) ---
// Clock rate must be set to 400kHz inside Wire initialization
#define PIN_I2C1_SDA            1
#define PIN_I2C1_SCL            2

// --- SPI Bus Configuration (Shared by Micro SD Card & DW1000 UWB Module) ---
#define PIN_SPI_MOSI            41
#define PIN_SPI_MISO            40
#define PIN_SPI_SCK             39
#define PIN_SD_CS               42  // SD Card Select
#define PIN_UWB_CS              38  // UWB Module Chip Select
#define PIN_UWB_RST             10  // UWB Hardware Reset
#define PIN_UWB_IRQ             5   // Safe non-strapping interrupt (Bypasses GPIO45 boot issue)

// --- DRV8833 Dual H-Bridge Motor Control Pins (Controlled via LEDC/MCPWM) ---
#define PIN_MOTOR_L_IN1         15
#define PIN_MOTOR_L_IN2         16
#define PIN_MOTOR_R_IN1         17
#define PIN_MOTOR_R_IN2         18
#define PIN_SWEEP_IN1           6
#define PIN_SWEEP_IN2           7

// --- Hardware Status & User Interfaces (Built-in DevKit features) ---
#define PIN_SYS_RGB_LED         48  // Internal WS2812 addressable RGB
#define PIN_BOOT_BUTTON         0   // Native strap pin connected to Boot Button

#endif // PIN_MAP_H
