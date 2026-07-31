#include "actuator/pump_driver.h"

#include <Arduino.h>

void PumpDriver::begin(uint8_t pin, bool active_high) {
  pin_ = pin;
  active_high_ = active_high;
  pinMode(pin_, OUTPUT);
  set(false);
}

void PumpDriver::set(bool on) {
  on_ = on;
  const bool level = active_high_ ? on : !on;
  digitalWrite(pin_, level ? HIGH : LOW);
}

bool PumpDriver::isOn() const { return on_; }
