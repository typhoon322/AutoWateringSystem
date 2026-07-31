#include "actuator/pca9555.h"

#include <Wire.h>

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
  if (!writeReg8(kRegConfig0, 0x00) || !writeReg8(kRegConfig1, 0x00)) {
    return false;
  }
  output_ = 0;
  return writeOutput(0);
}

bool Pca9555::writeOutput(uint16_t value) {
  output_ = value;
  return writeReg8(kRegOutput0, static_cast<uint8_t>(value & 0xFF)) &&
         writeReg8(kRegOutput1, static_cast<uint8_t>((value >> 8) & 0xFF));
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
