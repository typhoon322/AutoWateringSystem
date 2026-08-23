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

// ── OLED 屏（SH1106 128×64，I2C 共用 GPIO8/9，地址 0x3C）──
//   模块 IIC_SCL→PIN_I2C_SCL(9)、IIC_SDA→PIN_I2C_SDA(8)、3V3/GND 供电
//   OLED_I2C_ADDR 0x3C 已在 boards/board_io_map.h 定义，此处不重复
// ── 编码器/按键组（模块 TRIM_A/TRIM_B/KEY0/KEY1，线序用户自调）──
#define PIN_ENC_A    6    // TRIM_A
#define PIN_ENC_B    7    // TRIM_B
#define PIN_BTN_OK   16   // KEY1=确认
#define PIN_BTN_BACK 17   // KEY0=返回
// ── 屏幕分辨率 ──
#define LVGL_HOR_RES 128
#define LVGL_VER_RES 64
