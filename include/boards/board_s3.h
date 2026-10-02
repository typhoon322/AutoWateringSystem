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

// ── 0.96" SSD1306 128×64，I2C 四针，与扩展板共用 GPIO8/9，地址 0x3C ──
#define BOARD_HAS_STATUS_OLED 1
#define STATUS_OLED_W 128
#define STATUS_OLED_H 64

// 自复按钮：另一端接 GND，内部上拉，按下为低
#define PIN_BTN_ACTION 6  // 短按：检测并浇干盆；长按：切换自动/手动
#define PIN_BTN_ESTOP 7   // 按下即急停

// 旧 LVGL 编码器界面仍引用这些脚；当前量产交互不用编码器
#define PIN_ENC_A 6
#define PIN_ENC_B 7
#define PIN_BTN_OK 16
#define PIN_BTN_BACK 17
#define LVGL_HOR_RES 128
#define LVGL_VER_RES 64
