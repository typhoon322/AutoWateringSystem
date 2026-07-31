#pragma once

#include "types/zone_config.h"

class WebServerUi {
 public:
  void begin(SystemContextEx *ctx);
  void loop();

 private:
  void setupRoutes();
  void startWiFi();
  void handleStatus();
  void handleSettingsGet();
  void handleSettingsPost();
  void handleIrrigate();
  void handleEmergencyStop();
  void handleWifiGet();
  void handleWifiPost();
  void handleRoot();

  SystemContextEx *ctx_ = nullptr;
  uint32_t last_wifi_attempt_ms_ = 0;
  bool wifi_started_ = false;
};
