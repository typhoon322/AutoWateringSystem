#pragma once

#include "types/zone_config.h"

class IrrigationController;

class DisplayDriver {
 public:
  bool begin();
  void showSplash();
  void showStatus(const SystemStatus &status, const ZoneStatus *zones, uint8_t zone_count,
                  IrrigationController *controller, bool wifi_ap_active);

 private:
  bool initialized_ = false;
};
