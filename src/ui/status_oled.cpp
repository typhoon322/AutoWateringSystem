#include "ui/status_oled.h"

#if BOARD_HAS_STATUS_OLED

#include <Arduino.h>
#include <U8g2lib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "bus/i2c_bus.h"
#include "config.h"
#include "control/irrigation_controller.h"
#include "types/zone_config.h"

extern IrrigationController g_controller;

namespace {
U8G2_SSD1306_128X64_NONAME_F_HW_I2C g_oled(U8G2_R2, U8X8_PIN_NONE);

bool g_ok = false;
char g_note[24] = "待机";
uint32_t g_note_until_ms = 0;

const char *feelText(const ZoneStatus &zone, const ZoneConfig &cfg) {
  if (!zone.sensor_valid) {
    return "未接";
  }
  if (zone.moisture_pct < cfg.moisture_low) {
    return "偏干";
  }
  if (zone.moisture_pct > cfg.moisture_high) {
    return "偏湿";
  }
  return "正常";
}

bool formatAction(char *out, size_t cap, const SystemStatus &status) {
  const int z = status.active_zone >= 0 ? status.active_zone : status.active_valve;
  const int num = z + 1;
  switch (status.state) {
    case IrrigationState::Checking:
      snprintf(out, cap, "正在检测");
      return true;
    case IrrigationState::Valving:
    case IrrigationState::Pumping:
      if (num > 0) {
        snprintf(out, cap, "%d# 正在浇水作业", num);
      } else {
        snprintf(out, cap, "正在浇水作业");
      }
      return true;
    case IrrigationState::Done:
      if (num > 0) {
        snprintf(out, cap, "%d# 浇水完成", num);
      } else {
        snprintf(out, cap, "浇水完成");
      }
      return true;
    case IrrigationState::Fault:
      snprintf(out, cap, "故障锁定");
      return true;
    default:
      break;
  }
  if (status.purge_on) {
    snprintf(out, cap, "正在排气");
    return true;
  }
  if (status.pump_on && num > 0 && status.valve_on) {
    snprintf(out, cap, "%d# 正在浇水作业", num);
    return true;
  }
  if (status.pump_on) {
    snprintf(out, cap, "水泵运行中");
    return true;
  }
  if (status.valve_on && num > 0) {
    snprintf(out, cap, "%d# 阀已打开", num);
    return true;
  }
  return false;
}

void drawAction(const char *text) {
  g_oled.drawUTF8(0, 40, text);
}

bool clockReady() {
  return time(nullptr) >= 1700000000L;
}

void drawClockLine() {
  char line[28];
  if (!clockReady()) {
    snprintf(line, sizeof(line), "请连手机蓝牙对时");
  } else {
    time_t now = time(nullptr);
    struct tm ti;
    localtime_r(&now, &ti);
    snprintf(line, sizeof(line), "%02d-%02d %02d:%02d", ti.tm_mon + 1, ti.tm_mday, ti.tm_hour,
             ti.tm_min);
  }
  g_oled.drawUTF8(0, 14, line);
}

void drawZonePages(const ZoneStatus *zones, const ZoneConfig *configs, uint8_t zone_count) {
  static uint8_t page = 0;
  static uint32_t page_ms = 0;
  static bool ready = false;
  const uint32_t now = millis();
  const uint8_t pages = zone_count == 0 ? 1 : static_cast<uint8_t>((zone_count + 2) / 3);
  if (!ready) {
    page_ms = now;
    ready = true;
  }
  if (pages <= 1) {
    page = 0;
  } else if (now - page_ms >= 10000) {
    page_ms = now;
    page = static_cast<uint8_t>((page + 1) % pages);
  }
  if (page >= pages) {
    page = 0;
  }

  const uint8_t start = page * 3;
  for (uint8_t row = 0; row < 3; ++row) {
    const uint8_t i = start + row;
    if (i >= zone_count) {
      break;
    }
    char line[28];
    if (zones[i].sensor_valid && g_controller.zoneSoaking(i)) {
      snprintf(line, sizeof(line), "%u# 渗水 %u%%", static_cast<unsigned>(i + 1),
               zones[i].moisture_pct);
    } else if (zones[i].sensor_valid) {
      snprintf(line, sizeof(line), "%u# %s %u%% %s", static_cast<unsigned>(i + 1),
               feelText(zones[i], configs[i]), zones[i].moisture_pct,
               configs[i].auto_enabled ? "自动" : "手动");
    } else {
      snprintf(line, sizeof(line), "%u# 未接", static_cast<unsigned>(i + 1));
    }
    g_oled.drawUTF8(0, 30 + static_cast<int>(row) * 16, line);
  }
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
  (void)state_text;
  drawClockLine();

  if (millis() < g_note_until_ms && g_note[0] != '\0') {
    drawAction(g_note);
  } else if (formatAction(line, sizeof(line), status)) {
    drawAction(line);
  } else {
    drawZonePages(zones, configs, zone_count);
  }
  g_oled.sendBuffer();
}

#endif
