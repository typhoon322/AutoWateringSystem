#include "bus/i2c_bus.h"

#include <Wire.h>

#include "config.h"

void irrigationI2cBegin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);
}

bool irrigationI2cProbe(uint8_t addr7) {
  Wire.beginTransmission(addr7);
  return Wire.endTransmission() == 0;
}
