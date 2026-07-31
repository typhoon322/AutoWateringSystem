#pragma once

#include <stdint.h>

class Pca9555 {
 public:
  bool begin(uint8_t addr7);
  bool writeOutput(uint16_t value);
  bool setBit(uint8_t bit, bool high);
  uint16_t output() const { return output_; }

 private:
  bool writeReg8(uint8_t reg, uint8_t value);

  uint8_t addr7_ = 0;
  uint16_t output_ = 0;
};

extern Pca9555 g_pca9555;
