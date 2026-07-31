#include "sensor/moisture_sensor.h"

#include <Arduino.h>

MoistureSensor::MoistureSensor(uint8_t pin) : pin_(pin) {}

void MoistureSensor::begin() {
  pinMode(pin_, INPUT);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
}

bool MoistureSensor::read(uint16_t &adc_out, uint8_t &pct_out) const {
  const int raw = analogRead(pin_);
  if (raw < 0) {
    return false;
  }
  adc_out = static_cast<uint16_t>(raw);
  pct_out = 0;
  return true;
}

uint8_t MoistureSensor::adcToPercent(uint16_t adc, uint16_t cal_dry, uint16_t cal_wet) {
  if (cal_dry == cal_wet) {
    return 0;
  }
  int32_t pct;
  if (cal_dry > cal_wet) {
    pct = (static_cast<int32_t>(cal_dry - adc) * 100) / (cal_dry - cal_wet);
  } else {
    pct = (static_cast<int32_t>(adc - cal_dry) * 100) / (cal_wet - cal_dry);
  }
  if (pct < 0) {
    pct = 0;
  }
  if (pct > 100) {
    pct = 100;
  }
  return static_cast<uint8_t>(pct);
}
