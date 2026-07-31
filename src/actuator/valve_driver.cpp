#include "actuator/valve_driver.h"

#include "actuator/pca9555.h"
#include "config.h"

void ValveDriver::begin(uint8_t count, bool active_high) {
  active_high_ = active_high;
  count_ = count > MAX_ZONES ? MAX_ZONES : count;
  active_ = -1;
  closeAll();
}

void ValveDriver::writeZone(uint8_t zone, bool on) {
  if (zone >= count_) {
    return;
  }
  const bool level = active_high_ ? on : !on;
  g_pca9555.setBit(kValvePcaBit[zone], level);
}

void ValveDriver::open(uint8_t zone) {
  if (zone >= count_) {
    return;
  }
  closeAll();
  writeZone(zone, true);
  active_ = static_cast<int8_t>(zone);
}

void ValveDriver::closeAll() {
  for (uint8_t i = 0; i < count_; ++i) {
    writeZone(i, false);
  }
  active_ = -1;
}

bool ValveDriver::isOpen(uint8_t zone) const {
  return active_ >= 0 && static_cast<uint8_t>(active_) == zone;
}

int8_t ValveDriver::activeZone() const { return active_; }
