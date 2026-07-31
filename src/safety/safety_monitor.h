#pragma once

#include "types/zone_config.h"

class SafetyMonitor {
 public:
  void begin(uint32_t daily_limit_ml, uint16_t max_run_sec, uint8_t dry_run_sec);
  void setDailyLimit(uint32_t ml);
  void setMaxRunSec(uint16_t sec);
  void setDryRunSec(uint8_t sec);

  bool canAutoIrrigate(uint32_t daily_ml) const;
  bool checkTimeout(uint32_t pump_start_ms) const;
  bool checkDryRun(uint32_t pump_start_ms, uint32_t pulses) const;

  void lock(SafetyState reason);
  void unlock();
  bool isLocked() const;
  SafetyState state() const;
  const char *stateText() const;

  void addDailyMl(uint32_t ml);
  void resetDailyIfNewDay();
  uint32_t dailyMl() const;

 private:
  uint32_t daily_limit_ml_ = 2000;
  uint16_t max_run_sec_ = 60;
  uint8_t dry_run_sec_ = 3;
  SafetyState state_ = SafetyState::Ok;
  bool locked_ = false;
  uint32_t daily_ml_ = 0;
  int last_day_ = -1;
};
