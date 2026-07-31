#pragma once

#include <stdint.h>

class PumpDriver {
 public:
  void begin(uint8_t pin, bool active_high);
  void set(bool on);
  bool isOn() const;

 private:
  uint8_t pin_ = 255;
  bool active_high_ = true;
  bool on_ = false;
};
