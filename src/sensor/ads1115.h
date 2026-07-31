#pragma once

#include <stdint.h>

class Ads1115 {
 public:
  bool begin();
  bool readSingleEnded(uint8_t addr7, uint8_t channel, int16_t &raw_out);
  bool readScaled12(uint8_t addr7, uint8_t channel, uint16_t &adc12_out);
};

extern Ads1115 g_ads1115;
