#pragma once

#include <stdint.h>

enum class IrrigationState : uint8_t {
  Idle,
  Checking,
  Valving,
  Pumping,
  Done,
  Fault,
};

enum class SafetyState : uint8_t {
  Ok,
  Timeout,
  DryRun,
  DailyLimit,
  Locked,
};

enum class IrrigateTrigger : uint8_t {
  None,
  Threshold,
  Schedule,
  Manual,
};

struct ZoneConfig {
  char name[16];
  uint8_t moisture_low;
  uint8_t moisture_high;
  uint16_t volume_ml;
  bool auto_enabled;
  bool schedule_enabled;
  uint8_t schedule_hour;
  uint8_t schedule_minute;
  uint16_t cal_dry;
  uint16_t cal_wet;
  bool schedule_fired_today;
  bool window_override;             // 覆盖全局时间窗口
  uint8_t win_sh, win_sm;           // 覆盖窗口开始 HH:MM
  uint8_t win_eh, win_em;           // 覆盖窗口结束 HH:MM
};

struct SystemConfig {
  uint16_t max_run_sec;
  uint32_t daily_limit_ml;
  uint16_t pulses_per_liter;
  uint8_t dry_run_sec;
  uint8_t zone_count;
  bool wifi_enabled;
  char wifi_ssid[33];
  char wifi_pass[65];
  bool auto_window_enabled;         // 全局自动浇水时间窗口总开关
  uint8_t auto_win_sh, auto_win_sm; // 窗口开始（默认 17:00）
  uint8_t auto_win_eh, auto_win_em; // 窗口结束（默认 21:00）
};

struct ZoneStatus {
  uint8_t id;
  uint16_t moisture_adc;
  uint8_t moisture_pct;
  bool sensor_valid;
};

struct SystemStatus {
  IrrigationState state;
  SafetyState safety;
  bool pump_on;
  bool valve_on;
  int8_t active_zone;
  int8_t active_valve;
  uint32_t daily_ml;
  uint8_t queue_len;
  uint16_t session_ml;
  IrrigateTrigger trigger;
};

struct SystemContext {
  SystemConfig *config;
  ZoneConfig *zones;
  ZoneStatus *zone_status;
  SystemStatus *status;
};

class IrrigationController;

struct SystemContextEx {
  SystemConfig *config;
  ZoneConfig *zones;
  ZoneStatus *zone_status;
  SystemStatus *status;
  IrrigationController *controller;
};
