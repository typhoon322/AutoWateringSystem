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

// ── LCD 组（ST7789 SPI，6 线成束；线序按实际屏幕模块调整本组宏）──
#define PIN_LCD_SCLK 12   // J1-18
#define PIN_LCD_MOSI 11   // J1-17
#define PIN_LCD_CS   10   // J1-16
#define PIN_LCD_RST  13   // J1-19
#define PIN_LCD_DC   14   // J1-20
#define PIN_LCD_BLK  15   // J1-8（背光）
// ── 输入组（编码器+按钮，4 线成束）──
#define PIN_ENC_A    6    // J1-6
#define PIN_ENC_B    7    // J1-7
#define PIN_BTN_OK   16   // J1-9
#define PIN_BTN_BACK 17   // J1-10
// ── 屏幕分辨率 ──
#define LVGL_HOR_RES 320
#define LVGL_VER_RES 240
