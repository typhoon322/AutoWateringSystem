#pragma once

#include "boards/board_io_map.h"

// 01Space / Spotpear ESP32-C3 0.42" OLED (ESP32-C3FH4/FN4, 4 MB flash)
// Schematic: https://github.com/01Space/ESP32-C3-0.42LCD
//
// On-board OLED (SSD1306) shares I2C with expander daughterboard:
//   SDA GPIO5, SCL GPIO6 — OLED @0x3C, ADS1115×3, PCA9555 @0x20
//
// Do NOT use GPIO5/6 for anything except I2C on this board.

#define BOARD_NAME "ESP32-C3-0.42-OLED"
#define BOARD_HAS_OLED 1

// Shared I2C bus (on-board OLED + ADS1115 + PCA9555)
#define PIN_I2C_SDA 5
#define PIN_I2C_SCL 6
#define PIN_OLED_SDA PIN_I2C_SDA
#define PIN_OLED_SCL PIN_I2C_SCL

// MCU-direct actuators / sensors
#define PIN_PUMP_RELAY 3
#define PUMP_ACTIVE_HIGH 1
#define PIN_FLOW_METER 7  // GPIO5 = OLED SDA — do not use for flow meter

// On-board user LED (GPIO8), active LOW; WS2812 on GPIO2 if present (unused here)
#define PIN_STATUS_LED 8
#define BOARD_LED_ACTIVE_LOW 1
#define PIN_RGB_LED PIN_STATUS_LED  // alias for main.cpp status helper

#define VALVE_ACTIVE_HIGH 1
