#include "sensor/moisture_sensor.h"

#include "config.h"
#include "sensor/ads1115.h"

MoistureSensor::MoistureSensor(uint8_t zone_index) : zone_index_(zone_index) {}

void MoistureSensor::begin() {}

bool MoistureSensor::read(uint16_t &adc_out, uint8_t &pct_out) const {
  pct_out = 0;
  if (zone_index_ >= MAX_ZONES) {
    return false;
  }
  const AdsChannel &ch = kMoistureAds[zone_index_];
  return g_ads1115.readScaled12(ch.addr, ch.channel, adc_out);
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
