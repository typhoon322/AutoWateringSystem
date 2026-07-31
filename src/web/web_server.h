#pragma once

#include "types/zone_config.h"

class WebServerUi {
 public:
  void begin(SystemContextEx *ctx);
  void loop();

 private:
  void setupRoutes();
  void startWiFi();
  void handleRoot();
  void handleStatus();
  void handleSettingsGet();
  void handleSettingsPost();
  void handleIrrigate();
  void handleEmergencyStop();
  void handleStop();
  void handleTestPump();
  void handleTestValve();
  void handleCal();
  void handleFlow();
  void handleWifiGet();
  void handleWifiPost();
  void applyZoneCount(uint8_t n);

  SystemContextEx *ctx_ = nullptr;
  uint32_t last_wifi_attempt_ms_ = 0;
  bool wifi_started_ = false;
};
