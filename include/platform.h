#pragma once

/**
 * AutoIrrigation — Hardware Platform Policy
 *
 * Architecture: I2C expander daughterboard (swap MCU only between dev and prod)
 *   - 3× ADS1115 @ 0x48/0x49/0x4A  → up to 10 moisture probes
 *   - 1× PCA9555 @ 0x20            → up to 10 valve relays
 *   - Pump + flow meter on MCU GPIO
 *
 * Board variants (only MCU pins / build target differ):
 *
 *   BOARD_C3_OLED_TEST  — ESP32-C3 + OLED dev board (I2C GPIO6/7)
 *   BOARD_S3_IRRIGATION — ESP32-S3 production (I2C GPIO8/9)
 *
 * PlatformIO:
 *   pio run -e esp32-c3-oled-test    # 前期验证
 *   pio run -e esp32-s3-irrigation   # 量产
 */

#if defined(BOARD_C3_OLED_TEST)
#include "boards/board_c3_oled.h"
#elif defined(BOARD_S3_IRRIGATION)
#include "boards/board_s3.h"
#else
#error "Define BOARD_C3_OLED_TEST (dev) or BOARD_S3_IRRIGATION (prod)."
#endif
