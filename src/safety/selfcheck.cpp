#include "safety/selfcheck.h"

#include <Arduino.h>

#include "bus/i2c_bus.h"
#include "config.h"

SelfCheckResult g_selfcheck;

void run_selfcheck() {
  g_selfcheck = SelfCheckResult{};
  Serial.println(F("[selfcheck] start"));

  // PCA9555 阀扩展板 0x20
  g_selfcheck.pca9555_ok = irrigationI2cProbe(PCA9555_ADDR_VALVES);
  Serial.printf("[selfcheck] PCA9555(0x20): %s\n", g_selfcheck.pca9555_ok ? "PASS" : "FAIL");

  // ADS1115 ×3
  g_selfcheck.ads_ok = 0;
  for (uint8_t a : {ADS1115_ADDR_0, ADS1115_ADDR_1, ADS1115_ADDR_2}) {
    if (irrigationI2cProbe(a)) {
      ++g_selfcheck.ads_ok;
    }
  }
  Serial.printf("[selfcheck] ADS1115: %u/3\n", g_selfcheck.ads_ok);

  // 屏幕 0x3C（LVGL 或 OLED(U8g2) 构建均计入）
#if BOARD_HAS_LVGL || BOARD_HAS_OLED
  g_selfcheck.oled_ok = irrigationI2cProbe(OLED_I2C_ADDR);
  Serial.printf("[selfcheck] OLED(0x3C): %s\n", g_selfcheck.oled_ok ? "PASS" : "FAIL");
#endif

  g_selfcheck.warnings = (g_selfcheck.pca9555_ok ? 0 : 1) +
                         (g_selfcheck.ads_ok < 1 ? 1 : 0) +
                         (g_selfcheck.oled_ok ? 0 : 1);
  g_selfcheck.done = true;
  Serial.printf("[selfcheck] done, warnings=%u\n", g_selfcheck.warnings);
}
