#pragma once

#include "boards/board_io_map.h"

// ESP32-S3 — production target (same I2C daughterboard, swap MCU module only)

#define BOARD_NAME "ESP32-S3-Irrigation"
#define BOARD_HAS_OLED 0

#define PIN_I2C_SDA 8
#define PIN_I2C_SCL 9

#define PIN_PUMP_RELAY 4
#define PUMP_ACTIVE_HIGH 1
#define PIN_FLOW_METER 5
#define PIN_RGB_LED 48

#define VALVE_ACTIVE_HIGH 1
