#pragma once

// ESP32-C3-DevKitM-1 irrigation board
// GPIO11–17 are flash pins — do not use.

#define BOARD_NAME "ESP32-C3-Irrigation"

// Soil moisture (ADC1)
#define PIN_MOISTURE_Z0 0
#define PIN_MOISTURE_Z1 1
#define PIN_MOISTURE_Z2 2  // reserved
#define PIN_MOISTURE_Z3 4  // reserved

// Pump relay (active level set below)
#define PIN_PUMP_RELAY 3
#define PUMP_ACTIVE_HIGH 1

// Flow meter pulse input
#define PIN_FLOW_METER 5

// On-board RGB LED (WS2812) on DevKitM-1
#define PIN_RGB_LED 8

// Optional OLED (reserved)
#define PIN_OLED_SDA 6
#define PIN_OLED_SCL 7
#define OLED_I2C_ADDR 0x3C
