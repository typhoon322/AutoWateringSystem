#pragma once

#include <stdint.h>

/**
 * I2C expander layout for the ESP32-S3 board.
 *
 * Moisture: 3× ADS1115 @ 0x48/0x49/0x4A → 10 single-ended channels
 * Valves:   1× PCA9555 @ 0x20 → outputs P0–P9 (one NC valve relay each)
 * OLED:     0x3C on the same I2C bus (0.96" SSD1306, SDA=8 SCL=9)
 */

#define IRRIGATION_USE_I2C_EXPANDERS 1

#define ADS1115_ADDR_0 0x48
#define ADS1115_ADDR_1 0x49
#define ADS1115_ADDR_2 0x4A
#define PCA9555_ADDR_VALVES 0x20
#define OLED_I2C_ADDR 0x3C

#define BOARD_VALVE_COUNT 10

struct AdsChannel {
  uint8_t addr;
  uint8_t channel;  // 0–3 per chip
};

static constexpr AdsChannel kMoistureAds[MAX_ZONES] = {
    {ADS1115_ADDR_0, 0}, {ADS1115_ADDR_0, 1}, {ADS1115_ADDR_0, 2}, {ADS1115_ADDR_0, 3},
    {ADS1115_ADDR_1, 0}, {ADS1115_ADDR_1, 1}, {ADS1115_ADDR_1, 2}, {ADS1115_ADDR_1, 3},
    {ADS1115_ADDR_2, 0}, {ADS1115_ADDR_2, 1},
};

static constexpr uint8_t kValvePcaBit[MAX_ZONES] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
