#include "bus/i2c_bus.h"

#include <Arduino.h>
#include <Wire.h>

#include "config.h"

void irrigationI2cBegin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(100000);  // breadboard-friendly; raise to 400k when wiring is short/stable
  Wire.setTimeOut(50);    // 无 ACK/总线异常时 50ms 超时返回，避免长时间阻塞（2026-08-22 实测踩坑）
}

bool irrigationI2cProbe(uint8_t addr7) {
  Wire.beginTransmission(addr7);
  return Wire.endTransmission() == 0;
}

void irrigationI2cScan() {
  uint8_t found = 0;
  Serial.print(F("I2C scan SDA="));
  Serial.print(PIN_I2C_SDA);
  Serial.print(F(" SCL="));
  Serial.println(PIN_I2C_SCL);
  for (uint8_t addr = 1; addr < 127; addr++) {
    if (irrigationI2cProbe(addr)) {
      Serial.printf("  found 0x%02X\n", addr);
      found++;
    }
  }
  if (found == 0) {
    Serial.println(F("  (no devices)"));
  }
}
