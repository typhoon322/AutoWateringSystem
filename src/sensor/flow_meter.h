#pragma once

#include <stdint.h>

class FlowMeter {
 public:
  explicit FlowMeter(uint8_t pin);

  void begin();
  void resetSession();
  uint32_t pulses() const;
  uint16_t volumeMl(uint16_t pulses_per_liter) const;
  void onPulse();

 private:
  uint8_t pin_;
  volatile uint32_t pulses_;
};
