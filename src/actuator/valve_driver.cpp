#include "actuator/valve_driver.h"

#include "actuator/pca9555.h"
#include "config.h"

void ValveDriver::begin(uint8_t count, bool /*active_high*/) {
  count_ = count > MAX_ZONES ? MAX_ZONES : count;
  active_ = -1;
  closeAll();
}

void ValveDriver::open(uint8_t zone) {
  if (zone >= count_) {
    return;
  }
  closeAll();
  // 低电平触发模块：IN 拉低(0V) → 吸合
  g_pca9555.setDriveLow(kValvePcaBit[zone]);
  active_ = static_cast<int8_t>(zone);
}

void ValveDriver::closeAll() {
  for (uint8_t i = 0; i < count_; ++i) {
    // 高阻(悬空) → 释放。3.3V 输出高会让 5V 低电平触发模块半导通误吸合，
    // 故关断用高阻而非输出高。
    g_pca9555.setHighZ(kValvePcaBit[i]);
  }
  active_ = -1;
}

bool ValveDriver::isOpen(uint8_t zone) const {
  return active_ >= 0 && static_cast<uint8_t>(active_) == zone;
}

int8_t ValveDriver::activeZone() const { return active_; }
