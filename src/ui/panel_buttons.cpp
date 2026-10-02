#include "ui/panel_buttons.h"

#if BOARD_HAS_STATUS_OLED

#include <Arduino.h>
#include <stdio.h>

#include "config.h"
#include "control/irrigation_controller.h"
#include "control/zone_manager.h"
#include "safety/safety_monitor.h"
#include "storage/settings_store.h"
#include "types/zone_config.h"
#include "ui/status_oled.h"

extern ZoneManager g_zone_manager;
extern IrrigationController g_controller;
extern SafetyMonitor g_safety;
extern SettingsStore g_settings;
extern SystemConfig g_sys_config;
extern ZoneConfig g_zone_configs[MAX_ZONES];
extern ZoneStatus g_zone_status[MAX_ZONES];
extern SystemStatus g_sys_status;

namespace {
constexpr uint32_t kDebounceMs = 30;
constexpr uint32_t kLongPressMs = 800;

struct Btn {
  uint8_t pin;
  bool raw;
  bool stable;
  uint32_t change_ms;
  uint32_t press_ms;
  bool long_fired;
};

Btn g_action{PIN_BTN_ACTION, false, false, 0, 0, false};
Btn g_estop{PIN_BTN_ESTOP, false, false, 0, 0, false};

bool zoneIsDry(uint8_t i) {
  if (i >= g_sys_config.zone_count) {
    return false;
  }
  if (!g_zone_status[i].sensor_valid) {
    return false;
  }
  const ZoneConfig &zc = g_zone_configs[i];
  if (zc.cal_dry == DEFAULT_CAL_DRY || zc.cal_wet == DEFAULT_CAL_WET || zc.cal_dry == zc.cal_wet) {
    return false;
  }
  return g_zone_status[i].moisture_pct < zc.moisture_low;
}

bool anyCalibrated() {
  for (uint8_t i = 0; i < g_sys_config.zone_count; ++i) {
    const ZoneConfig &zc = g_zone_configs[i];
    if (zc.cal_dry != DEFAULT_CAL_DRY && zc.cal_wet != DEFAULT_CAL_WET && zc.cal_dry != zc.cal_wet) {
      return true;
    }
  }
  return false;
}

void onActionShort() {
  if (g_safety.isLocked() || g_sys_status.state == IrrigationState::Fault) {
    status_oled_set_note("故障锁定");
    Serial.println(F("BTN: action ignored, fault locked"));
    return;
  }
  g_zone_manager.sampleAll();
  if (!anyCalibrated()) {
    status_oled_set_note("请先标定");
    Serial.println(F("BTN: no calibration"));
    return;
  }
  uint8_t queued = 0;
  for (uint8_t i = 0; i < g_sys_config.zone_count; ++i) {
    if (!zoneIsDry(i)) {
      continue;
    }
    if (g_controller.requestManual(i, g_zone_configs[i].volume_ml)) {
      ++queued;
    }
  }
  char note[24];
  if (queued == 0) {
    status_oled_set_note("无需浇水");
    Serial.println(F("BTN: sample done, no dry zone"));
  } else {
    snprintf(note, sizeof(note), "排队 %u 盆", queued);
    status_oled_set_note(note);
    Serial.printf("BTN: queued %u zone(s)\n", queued);
  }
}

void onActionLong() {
  bool any_auto = false;
  for (uint8_t i = 0; i < g_sys_config.zone_count; ++i) {
    if (g_zone_configs[i].auto_enabled) {
      any_auto = true;
      break;
    }
  }
  const bool enable = !any_auto;
  for (uint8_t i = 0; i < g_sys_config.zone_count; ++i) {
    g_zone_configs[i].auto_enabled = enable;
  }
  SystemContext sc = {&g_sys_config, g_zone_configs, g_zone_status, &g_sys_status};
  g_settings.save(sc);
  status_oled_set_note(enable ? "已切自动" : "已切手动");
  Serial.printf("BTN: mode %s\n", enable ? "AUTO" : "MANUAL");
}

void onEstop() {
  g_controller.emergencyStop();
  status_oled_set_note("急停");
  Serial.println(F("BTN: emergency stop"));
}

void pollBtn(Btn &btn, uint32_t now, void (*on_press)(), void (*on_short)(), void (*on_long)()) {
  const bool raw = digitalRead(btn.pin) == LOW;
  if (raw != btn.raw) {
    btn.raw = raw;
    btn.change_ms = now;
  }
  if (now - btn.change_ms < kDebounceMs) {
    return;
  }
  if (btn.stable == raw) {
    if (btn.stable && on_long != nullptr && !btn.long_fired && now - btn.press_ms >= kLongPressMs) {
      btn.long_fired = true;
      on_long();
    }
    return;
  }
  btn.stable = raw;
  if (btn.stable) {
    btn.press_ms = now;
    btn.long_fired = false;
    if (on_press != nullptr) {
      on_press();
    }
  } else if (on_short != nullptr && !btn.long_fired) {
    on_short();
  }
}
}  // namespace

void panel_buttons_begin() {
  pinMode(PIN_BTN_ACTION, INPUT_PULLUP);
  pinMode(PIN_BTN_ESTOP, INPUT_PULLUP);
  const uint32_t now = millis();
  g_action.raw = digitalRead(PIN_BTN_ACTION) == LOW;
  g_action.stable = g_action.raw;
  g_action.change_ms = now;
  g_estop.raw = digitalRead(PIN_BTN_ESTOP) == LOW;
  g_estop.stable = g_estop.raw;
  g_estop.change_ms = now;
}

void panel_buttons_poll() {
  const uint32_t now = millis();
  pollBtn(g_action, now, nullptr, onActionShort, onActionLong);
  pollBtn(g_estop, now, onEstop, nullptr, nullptr);
}

#endif
