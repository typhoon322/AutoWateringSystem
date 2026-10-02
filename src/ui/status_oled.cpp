#include "ui/status_oled.h"

#if BOARD_HAS_STATUS_OLED

#include <Arduino.h>
#include <U8g2lib.h>
#include <stdio.h>
#include <string.h>

#include "bus/i2c_bus.h"
#include "config.h"
#include "types/zone_config.h"

namespace {
U8G2_SSD1306_128X64_NONAME_F_HW_I2C g_oled(U8G2_R0, U8X8_PIN_NONE);

bool g_ok = false;
char g_note[24] = "待机";
uint32_t g_note_until_ms = 0;

const char *modeText(const ZoneConfig *configs, uint8_t zone_count) {
  for (uint8_t i = 0; i < zone_count; ++i) {
    if (configs[i].auto_enabled) {
      return "自动";
    }
  }
  return "手动";
}
}  // namespace

bool status_oled_begin() {
  irrigationI2cBegin();
  delay(20);
  if (!irrigationI2cProbe(OLED_I2C_ADDR)) {
    g_ok = false;
    return false;
  }
  g_oled.begin();
  g_oled.setBusClock(100000);
  g_oled.setContrast(255);
  g_oled.clearBuffer();
  g_oled.setFont(u8g2_font_wqy12_t_gb2312);
  g_oled.drawUTF8(0, 14, "AutoIrrigation");
  g_oled.drawUTF8(0, 30, "128x64 就绪");
  g_oled.sendBuffer();
  g_ok = true;
  return true;
}

void status_oled_set_note(const char *text) {
  if (text == nullptr) {
    return;
  }
  strncpy(g_note, text, sizeof(g_note) - 1);
  g_note[sizeof(g_note) - 1] = '\0';
  g_note_until_ms = millis() + 4000;
}

void status_oled_show(const SystemStatus &status, const ZoneStatus *zones,
                      const ZoneConfig *configs, uint8_t zone_count, const char *state_text) {
  if (!g_ok) {
    return;
  }
  char line[28];
  g_oled.clearBuffer();
  g_oled.setFont(u8g2_font_wqy12_t_gb2312);

  const char *st = "空闲";
  switch (status.state) {
    case IrrigationState::Valving:
      st = "开阀";
      break;
    case IrrigationState::Pumping:
      st = "浇水";
      break;
    case IrrigationState::Done:
      st = "完成";
      break;
    case IrrigationState::Fault:
      st = "故障";
      break;
    case IrrigationState::Checking:
      st = "检测";
      break;
    default:
      st = "空闲";
      break;
  }
  (void)state_text;
  g_oled.drawUTF8(0, 12, st);

  const char *valve = "关";
  if (status.valve_on && status.active_valve >= 0) {
    snprintf(line, sizeof(line), "泵%s 阀%u %s", status.pump_on ? "开" : "关",
             static_cast<unsigned>(status.active_valve), modeText(configs, zone_count));
  } else {
    snprintf(line, sizeof(line), "泵%s 阀%s %s", status.pump_on ? "开" : "关", valve,
             modeText(configs, zone_count));
  }
  g_oled.drawUTF8(0, 26, line);

  const uint8_t n = zone_count > 4 ? 4 : zone_count;
  char zline[28] = "";
  size_t used = 0;
  for (uint8_t i = 0; i < n && i < 2; ++i) {
    char part[12];
    if (zones[i].sensor_valid) {
      snprintf(part, sizeof(part), "%u:%u%% ", i, zones[i].moisture_pct);
    } else {
      snprintf(part, sizeof(part), "%u:-- ", i);
    }
    const size_t len = strlen(part);
    if (used + len < sizeof(zline)) {
      memcpy(zline + used, part, len + 1);
      used += len;
    }
  }
  g_oled.drawUTF8(0, 40, zline);

  zline[0] = '\0';
  used = 0;
  for (uint8_t i = 2; i < n; ++i) {
    char part[12];
    if (zones[i].sensor_valid) {
      snprintf(part, sizeof(part), "%u:%u%% ", i, zones[i].moisture_pct);
    } else {
      snprintf(part, sizeof(part), "%u:-- ", i);
    }
    const size_t len = strlen(part);
    if (used + len < sizeof(zline)) {
      memcpy(zline + used, part, len + 1);
      used += len;
    }
  }
  if (zone_count > 4) {
    g_oled.drawUTF8(90, 54, "...");
  }
  g_oled.drawUTF8(0, 54, zline);

  const char *note = (millis() < g_note_until_ms) ? g_note : "待机";
  g_oled.drawUTF8(0, 63, note);
  g_oled.sendBuffer();
}

#endif
