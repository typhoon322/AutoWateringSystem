#include "sensor/ads1115.h"

#include <Wire.h>

#include "bus/i2c_bus.h"
#include "config.h"

namespace {
constexpr uint8_t kRegConversion = 0x00;
constexpr uint8_t kRegConfig = 0x01;

constexpr uint16_t kCfgOsSingle = 0x8000;
constexpr uint16_t kCfgPga4096 = 0x0200;
constexpr uint16_t kCfgModeSingle = 0x0100;
constexpr uint16_t kCfgDr128sps = 0x0080;

bool writeReg16(uint8_t addr7, uint8_t reg, uint16_t value) {
  Wire.beginTransmission(addr7);
  Wire.write(reg);
  Wire.write(static_cast<uint8_t>(value >> 8));
  Wire.write(static_cast<uint8_t>(value & 0xFF));
  return Wire.endTransmission() == 0;
}

bool readReg16(uint8_t addr7, uint8_t reg, uint16_t &value_out) {
  Wire.beginTransmission(addr7);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(addr7, static_cast<uint8_t>(2)) != 2) {
    return false;
  }
  const uint8_t hi = Wire.read();
  const uint8_t lo = Wire.read();
  value_out = static_cast<uint16_t>((hi << 8) | lo);
  return true;
}
}  // namespace

Ads1115 g_ads1115;

bool Ads1115::begin() {
  irrigationI2cBegin();
  const uint8_t addrs[] = {ADS1115_ADDR_0, ADS1115_ADDR_1, ADS1115_ADDR_2};
  bool any = false;
  for (uint8_t addr : addrs) {
    if (irrigationI2cProbe(addr)) {
      any = true;
    }
  }
  return any;
}

bool Ads1115::readSingleEnded(uint8_t addr7, uint8_t channel, int16_t &raw_out) {
  if (channel > 3) {
    return false;
  }
  const uint16_t mux = static_cast<uint16_t>((4 + channel) << 12);
  const uint16_t cfg = kCfgOsSingle | mux | kCfgPga4096 | kCfgModeSingle | kCfgDr128sps;
  if (!writeReg16(addr7, kRegConfig, cfg)) {
    return false;
  }
  delay(12);
  uint16_t raw_u = 0;
  if (!readReg16(addr7, kRegConversion, raw_u)) {
    return false;
  }
  raw_out = static_cast<int16_t>(raw_u);
  return true;
}

bool Ads1115::readScaled12(uint8_t addr7, uint8_t channel, uint16_t &adc12_out) {
  int16_t raw = 0;
  if (!readSingleEnded(addr7, channel, raw)) {
    return false;
  }
  if (raw < 0) {
    raw = 0;
  }
  adc12_out = static_cast<uint16_t>((static_cast<uint32_t>(raw) * 4095U) / 32767U);
  return true;
}
