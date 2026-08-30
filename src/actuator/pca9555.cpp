#include "actuator/pca9555.h"

#include <Wire.h>

#include "bus/i2c_bus.h"
#include "config.h"

namespace {
constexpr uint8_t kRegOutput0 = 0x02;
constexpr uint8_t kRegOutput1 = 0x03;
constexpr uint8_t kRegConfig0 = 0x06;
constexpr uint8_t kRegConfig1 = 0x07;
}  // namespace

Pca9555 g_pca9555;

bool Pca9555::writeReg8(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(addr7_);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool Pca9555::begin(uint8_t addr7) {
  addr7_ = addr7;
  ok_ = false;
  for (uint8_t attempt = 0; attempt < 5; ++attempt) {
    if (!irrigationI2cProbe(addr7_)) {
      delay(20);
      continue;
    }
    // 初始化为全高阻（输入模式），避免启动瞬间输出 0V 使低电平触发模块误吸合。
    if (writeReg8(kRegConfig0, 0xFF) && writeReg8(kRegConfig1, 0xFF)) {
      config_ = 0xFFFF;
      output_ = 0;
      ok_ = true;
      return true;
    }
    delay(20);
  }
  return false;
}

bool Pca9555::writeOutput(uint16_t value) {
  if (!ok_) {
    return false;  // 离线：短路，避免 I2C 无 ACK 阻塞
  }
  output_ = value;
  return writeReg8(kRegOutput0, static_cast<uint8_t>(value & 0xFF)) &&
         writeReg8(kRegOutput1, static_cast<uint8_t>((value >> 8) & 0xFF));
}

bool Pca9555::writeConfig() {
  if (!ok_) {
    return false;
  }
  return writeReg8(kRegConfig0, static_cast<uint8_t>(config_ & 0xFF)) &&
         writeReg8(kRegConfig1, static_cast<uint8_t>((config_ >> 8) & 0xFF));
}

bool Pca9555::setBit(uint8_t bit, bool high) {
  if (bit > 15) {
    return false;
  }
  uint16_t next = output_;
  if (high) {
    next |= static_cast<uint16_t>(1U << bit);
  } else {
    next &= static_cast<uint16_t>(~(1U << bit));
  }
  return writeOutput(next);
}

bool Pca9555::setHighZ(uint8_t bit) {
  if (bit > 15) {
    return false;
  }
  config_ |= static_cast<uint16_t>(1U << bit);
  return writeConfig();
}

bool Pca9555::setDriveLow(uint8_t bit) {
  if (bit > 15) {
    return false;
  }
  config_ &= static_cast<uint16_t>(~(1U << bit));  // 输出模式
  output_ &= static_cast<uint16_t>(~(1U << bit));  // 输出 0V
  return writeConfig() && writeOutput(output_);
}
