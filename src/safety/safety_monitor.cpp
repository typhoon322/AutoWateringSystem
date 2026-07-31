#include "safety/safety_monitor.h"

#include <Arduino.h>
#include <time.h>

void SafetyMonitor::begin(uint32_t daily_limit_ml, uint16_t max_run_sec, uint8_t dry_run_sec) {
  daily_limit_ml_ = daily_limit_ml;
  max_run_sec_ = max_run_sec;
  dry_run_sec_ = dry_run_sec;
  state_ = SafetyState::Ok;
  locked_ = false;
  daily_ml_ = 0;
  last_day_ = -1;
}

void SafetyMonitor::setDailyLimit(uint32_t ml) { daily_limit_ml_ = ml; }
void SafetyMonitor::setMaxRunSec(uint16_t sec) { max_run_sec_ = sec; }
void SafetyMonitor::setDryRunSec(uint8_t sec) { dry_run_sec_ = sec; }

bool SafetyMonitor::canAutoIrrigate(uint32_t daily_ml) const {
  if (locked_) {
    return false;
  }
  if (daily_limit_ml_ > 0 && daily_ml >= daily_limit_ml_) {
    return false;
  }
  return true;
}

bool SafetyMonitor::checkTimeout(uint32_t pump_start_ms) const {
  if (max_run_sec_ == 0) {
    return false;
  }
  return (millis() - pump_start_ms) >= static_cast<uint32_t>(max_run_sec_) * 1000U;
}

bool SafetyMonitor::checkDryRun(uint32_t pump_start_ms, uint32_t pulses) const {
  if (dry_run_sec_ == 0) {
    return false;
  }
  const uint32_t elapsed = millis() - pump_start_ms;
  return elapsed >= static_cast<uint32_t>(dry_run_sec_) * 1000U && pulses == 0;
}

void SafetyMonitor::lock(SafetyState reason) {
  locked_ = true;
  state_ = reason;
}

void SafetyMonitor::unlock() {
  locked_ = false;
  state_ = SafetyState::Ok;
}

bool SafetyMonitor::isLocked() const { return locked_; }
SafetyState SafetyMonitor::state() const { return state_; }

const char *SafetyMonitor::stateText() const {
  switch (state_) {
    case SafetyState::Ok:
      return "OK";
    case SafetyState::Timeout:
      return "TIMEOUT";
    case SafetyState::DryRun:
      return "DRY_RUN";
    case SafetyState::DailyLimit:
      return "DAILY_LIMIT";
    case SafetyState::Locked:
      return "LOCKED";
    default:
      return "UNKNOWN";
  }
}

void SafetyMonitor::addDailyMl(uint32_t ml) { daily_ml_ += ml; }

void SafetyMonitor::resetDailyIfNewDay() {
  time_t now;
  time(&now);
  struct tm ti;
  localtime_r(&now, &ti);
  const int day = ti.tm_yday;
  if (last_day_ >= 0 && day != last_day_) {
    daily_ml_ = 0;
  }
  last_day_ = day;
}

uint32_t SafetyMonitor::dailyMl() const { return daily_ml_; }
