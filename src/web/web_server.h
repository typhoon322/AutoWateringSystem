#pragma once

#include <cstddef>
#include <stdint.h>

#include "types/zone_config.h"

class WebServerUi {
 public:
  void begin(SystemContextEx *ctx);
  void loop();

 private:
  void setupRoutes();
  void startWiFi();
  void restartWiFi();
  void tickWiFi();
  void resolveWifiCredentials(char *ssid, size_t ssid_len, char *pass, size_t pass_len) const;
  void handleRoot();
  void handleDev();
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
  void handleFlowReset();
  void handleI2cScan();
  void handleSample();
  void handleAuto();
  void handleWifiGet();
  void handleWifiPost();
  void handleHistory();
  void applyZoneCount(uint8_t n);

  SystemContextEx *ctx_ = nullptr;
  uint32_t last_wifi_attempt_ms_ = 0;
  uint32_t wifi_connect_start_ms_ = 0;
  bool wifi_started_ = false;
  bool wifi_connect_pending_ = false;
};
