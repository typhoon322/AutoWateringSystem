#include "actuator/pump_driver.h"

#include <Arduino.h>

void PumpDriver::begin(uint8_t pin, bool /*active_high*/) {
  pin_ = pin;
  pinMode(pin_, OUTPUT);
  set(false);
}

void PumpDriver::set(bool on) {
  on_ = on;
  if (on) {
    // 低电平触发模块：IN 拉低(0V) → 吸合
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
  } else {
    // 高阻(悬空) → 释放。3.3V 输出高会让 5V 低电平触发模块半导通误吸合，
    // 故关断用高阻而非输出高。
    pinMode(pin_, INPUT);
  }
}

bool PumpDriver::isOn() const { return on_; }
