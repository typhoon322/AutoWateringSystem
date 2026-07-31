#pragma once

#include "sensor/moisture_sensor.h"
#include "types/zone_config.h"

class ZoneManager {
 public:
  ZoneManager(MoistureSensor *sensors, ZoneConfig *configs, ZoneStatus *status, uint8_t count);

  void begin();
  void sampleAll();
  bool needsWater(uint8_t zone_id) const;
  bool isSensorValid(uint8_t zone_id) const;
  uint8_t count() const;

 private:
  MoistureSensor *sensors_;
  ZoneConfig *configs_;
  ZoneStatus *status_;
  uint8_t count_;
};
