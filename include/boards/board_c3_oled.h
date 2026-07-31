#pragma once

#include "boards/board_io_map.h"

// ESP32-C3 + OLED — development / validation (same expander daughterboard as S3 prod)
// I2C bus shared: OLED @0x3C, ADS1115×3, PCA9555

#define BOARD_NAME "ESP32-C3-OLED-Exp"
#define BOARD_HAS_OLED 1

#define PIN_I2C_SDA 6
#define PIN_I2C_SCL 7

// MCU-direct actuators / sensors (not on expander)
#define PIN_PUMP_RELAY 3
#define PUMP_ACTIVE_HIGH 1
#define PIN_FLOW_METER 5
#define PIN_RGB_LED 8

#define VALVE_ACTIVE_HIGH 1
