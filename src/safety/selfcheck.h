#pragma once

#include <stdint.h>

// 开机自检结果（I2C 设备清单）；done=false 期间系统锁定写操作
struct SelfCheckResult {
  bool pca9555_ok;  // 阀扩展板 0x20
  uint8_t ads_ok;   // ADS1115 数量 0-3（0x48/0x49/0x4A）
  bool oled_ok;     // SH1106 0x3C
  uint8_t warnings; // 缺失项数
  bool done;        // 自检完成（锁定解除）
};

extern SelfCheckResult g_selfcheck;

// setup 中同步执行（Web/CLI 服务启动前）
void run_selfcheck();
