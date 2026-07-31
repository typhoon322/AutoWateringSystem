#include "control/zone_manager.h"

#include "config.h"

ZoneManager::ZoneManager(MoistureSensor *sensors, ZoneConfig *configs, ZoneStatus *status,
                         uint8_t count)
    : sensors_(sensors), configs_(configs), status_(status), count_(count), max_count_(count) {}

void ZoneManager::setCount(uint8_t count) {
  if (count < 1) {
    count = 1;
  }
  if (count > max_count_) {
    count = max_count_;
  }
  count_ = count;
}

void ZoneManager::begin() {
  for (uint8_t i = 0; i < count_; ++i) {
    sensors_[i].begin();
    status_[i].id = i;
    status_[i].moisture_adc = 0;
    status_[i].moisture_pct = 0;
    status_[i].sensor_valid = false;
  }
}

void ZoneManager::sampleAll() {
  for (uint8_t i = 0; i < count_; ++i) {
    uint16_t adc = 0;
    uint8_t pct = 0;
    if (sensors_[i].read(adc, pct)) {
      status_[i].moisture_adc = adc;
      status_[i].moisture_pct =
          MoistureSensor::adcToPercent(adc, configs_[i].cal_dry, configs_[i].cal_wet);
      status_[i].sensor_valid =
          adc >= ADC_MIN_VALID && adc <= ADC_MAX_VALID;
    } else {
      status_[i].sensor_valid = false;
    }
  }
}

bool ZoneManager::needsWater(uint8_t zone_id) const {
  if (zone_id >= count_) {
    return false;
  }
  if (!configs_[zone_id].auto_enabled) {
    return false;
  }
  if (!status_[zone_id].sensor_valid) {
    return false;
  }
  return status_[zone_id].moisture_pct < configs_[zone_id].moisture_low;
}

bool ZoneManager::isSensorValid(uint8_t zone_id) const {
  if (zone_id >= count_) {
    return false;
  }
  return status_[zone_id].sensor_valid;
}

uint8_t ZoneManager::count() const { return count_; }
