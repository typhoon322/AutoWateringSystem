#pragma once

#include <stdint.h>

class MoistureSensor {
 public:
  explicit MoistureSensor(uint8_t pin);

  void begin();
  bool read(uint16_t &adc_out, uint8_t &pct_out) const;
  static uint8_t adcToPercent(uint16_t adc, uint16_t cal_dry, uint16_t cal_wet);

 private:
  uint8_t pin_;
};
