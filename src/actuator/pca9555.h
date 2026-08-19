#pragma once

#include <stdint.h>

class Pca9555 {
 public:
  bool begin(uint8_t addr7);
  bool writeOutput(uint16_t value);
  bool setBit(uint8_t bit, bool high);
  // 高阻关断：引脚切为输入（悬空）。适配"低电平触发 + 悬空=释放"的 5V 光耦
  // 继电器模块——3.3V 输出高会让这类模块半导通误吸合，故关断用高阻而非输出高。
  bool setHighZ(uint8_t bit);
  // 驱动吸合：引脚切为输出并拉低（0V）。
  bool setDriveLow(uint8_t bit);
  uint16_t output() const { return output_; }

 private:
  bool writeReg8(uint8_t reg, uint8_t value);
  bool writeConfig();

  uint8_t addr7_ = 0;
  uint16_t output_ = 0;
  uint16_t config_ = 0xFFFF;  // 1=输入(高阻)，0=输出；上电默认全输入
};

extern Pca9555 g_pca9555;
