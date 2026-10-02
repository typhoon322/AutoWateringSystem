#pragma once

/**
 * AutoIrrigation — ESP32-S3
 *
 * I2C expander daughterboard:
 *   - 3× ADS1115 @ 0x48/0x49/0x4A  → up to 10 moisture probes
 *   - 1× PCA9555 @ 0x20            → up to 10 valve relays
 *   - Pump + flow meter on MCU GPIO
 *
 * PlatformIO: pio run -e esp32-s3-irrigation
 */

#include "boards/board_s3.h"
