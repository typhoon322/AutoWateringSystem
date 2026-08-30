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
  // 设备在线标志（begin 成功为 true）；离线时所有写操作短路，避免 I2C 无 ACK 阻塞
  bool available() const { return ok_; }
  uint16_t output() const { return output_; }

 private:
  bool writeReg8(uint8_t reg, uint8_t value);
  bool writeConfig();

  uint8_t addr7_ = 0;
  uint16_t output_ = 0;
  uint16_t config_ = 0xFFFF;  // 1=输入(高阻)，0=输出；上电默认全输入
  bool ok_ = false;
};

extern Pca9555 g_pca9555;
