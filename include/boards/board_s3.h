#pragma once

#include "boards/board_io_map.h"

// ESP32-S3 — production target (same I2C daughterboard, swap MCU module only)

#define BOARD_NAME "ESP32-S3-Irrigation"
#define BOARD_HAS_OLED 0

#define PIN_I2C_SDA 8
#define PIN_I2C_SCL 9

#define PIN_PUMP_RELAY 4
// 2026-08-18: 5V 光耦继电器模块为"低电平触发"（IN 接地吸合）。
// 3.3V 高电平带不动 5V 高电平触发模块（吸合无力/触点虚接），故用拉低触发。
#define PUMP_ACTIVE_HIGH 0
#define PIN_FLOW_METER 5
#define PIN_STATUS_LED 48
#define PIN_RGB_LED PIN_STATUS_LED
#define BOARD_LED_ACTIVE_LOW 0

#define VALVE_ACTIVE_HIGH 0  // 低电平触发（同泵继电器，见上）
