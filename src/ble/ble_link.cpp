#include "ble/ble_link.h"

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <stdio.h>
#include <string.h>

#include "actuator/pump_driver.h"
#include "actuator/valve_driver.h"
#include "config.h"
#include "control/irrigation_controller.h"
#include "control/zone_manager.h"
#include "safety/safety_monitor.h"
#include "safety/selfcheck.h"
#include "storage/settings_store.h"
#include "ui/status_oled.h"
#include "web/web_server.h"
#include "sensor/flow_meter.h"
#include "storage/irrigation_history.h"

#include <WiFi.h>
#include <sys/time.h>
#include <time.h>

extern IrrigationController g_controller;
extern ZoneManager g_zone_manager;
extern SafetyMonitor g_safety;
extern SettingsStore g_settings;
extern PumpDriver g_pump;
extern ValveDriver g_valves;
extern SystemConfig g_sys_config;
extern ZoneConfig g_zone_configs[MAX_ZONES];
extern ZoneStatus g_zone_status[MAX_ZONES];
extern SystemStatus g_sys_status;
extern WebServerUi g_web;
extern FlowMeter g_flow;
extern IrrigationHistory g_history;
extern void purgeSet(bool on);

namespace {

// 8a1f0001-4b2c-4d5e-9f10-112233445501 服务
// 8a1f0002-...502 状态通知（分包）
// 8a1f0003-...503 命令写入
// 8a1f0004-...504 应答通知
constexpr char kSvc[] = "8a1f0001-4b2c-4d5e-9f10-112233445501";
constexpr char kStatus[] = "8a1f0002-4b2c-4d5e-9f10-112233445502";
constexpr char kCmd[] = "8a1f0003-4b2c-4d5e-9f10-112233445503";
constexpr char kReply[] = "8a1f0004-4b2c-4d5e-9f10-112233445504";

constexpr uint8_t kChunkPayload = 17;
constexpr uint32_t kNotifyMs = 1000;

BLECharacteristic *g_status = nullptr;
BLECharacteristic *g_reply = nullptr;
BLEServer *g_server = nullptr;

bool g_inited = false;
bool g_connected = false;
uint32_t g_last_notify_ms = 0;

char g_cmd[96];
volatile bool g_cmd_pending = false;
portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;

void saveSettings() {
  SystemContext sc = {&g_sys_config, g_zone_configs, g_zone_status, &g_sys_status};
  g_settings.save(sc);
}

void reply(const char *text) {
  if (g_reply == nullptr || !g_connected) {
    return;
  }
  g_reply->setValue(text);
  g_reply->notify();
}

bool zoneOk(int z) { return z >= 0 && z < g_sys_config.zone_count; }

bool hmOk(int h, int m) { return h >= 0 && h <= 23 && m >= 0 && m <= 59; }

void copyName(char *dst, size_t cap, const char *src) {
  size_t n = 0;
  while (src[n] != '\0' && n + 1 < cap) {
    const uint8_t c = static_cast<uint8_t>(src[n]);
    size_t need = 1;
    if ((c & 0xE0) == 0xC0) {
      need = 2;
    } else if ((c & 0xF0) == 0xE0) {
      need = 3;
    } else if ((c & 0xF8) == 0xF0) {
      need = 4;
    }
    if (n + need >= cap) {
      break;
    }
    bool ok = true;
    for (size_t i = 0; i < need; ++i) {
      if (src[n + i] == '\0') {
        ok = false;
      }
    }
    if (!ok) {
      break;
    }
    n += need;
  }
  memcpy(dst, src, n);
  dst[n] = '\0';
}

void replyWifi() {
  char line[80];
  const bool up = WiFi.status() == WL_CONNECTED;
  snprintf(line, sizeof(line), "W %d %s", g_sys_config.wifi_enabled ? 1 : 0,
           up ? WiFi.localIP().toString().c_str() : "-");
  reply(line);
  snprintf(line, sizeof(line), "S %s", g_sys_config.wifi_ssid);
  reply(line);
}

void replyHistory() {
  const uint8_t n = g_history.count();
  char line[64];
  snprintf(line, sizeof(line), "H %u", n);
  reply(line);
  for (uint8_t i = 0; i < n; ++i) {
    const IrrigationRecord *r = g_history.get(i);
    if (r == nullptr) {
      continue;
    }
    snprintf(line, sizeof(line), "R %lu %u %u %u %u %lu", static_cast<unsigned long>(r->ts), r->zone,
             r->volume_ml, r->trigger, r->outcome, static_cast<unsigned long>(r->seq));
    reply(line);
    delay(20);
  }
}

void handleCommand(char *line) {
  while (*line == ' ') {
    ++line;
  }
  if (!g_selfcheck.done && strcmp(line, "estop") != 0 && strcmp(line, "stop") != 0 &&
      strncmp(line, "time ", 5) != 0 && strcmp(line, "hist") != 0 && strcmp(line, "wifi?") != 0) {
    reply("ERR selfcheck");
    return;
  }

  int a = 0;
  int b = 0;
  if (strcmp(line, "estop") == 0) {
    g_controller.emergencyStop();
    reply("OK");
    return;
  }
  if (strcmp(line, "stop") == 0) {
    g_controller.stop();
    reply("OK");
    return;
  }
  if (strcmp(line, "sample") == 0) {
    g_zone_manager.sampleAll();
    reply("OK");
    return;
  }
  if (strcmp(line, "detect") == 0) {
    const IrrigationController::DetectOutcome out = g_controller.detectAndWater();
    char msg[24];
    switch (out.code) {
      case IrrigationController::DetectCode::Locked:
#if BOARD_HAS_STATUS_OLED
        status_oled_set_note("故障锁定");
#endif
        reply("ERR lock");
        return;
      case IrrigationController::DetectCode::Uncalibrated:
#if BOARD_HAS_STATUS_OLED
        status_oled_set_note("请先标定");
#endif
        reply("ERR cal");
        return;
      case IrrigationController::DetectCode::Done:
        if (out.queued == 0) {
#if BOARD_HAS_STATUS_OLED
          status_oled_set_note("无需浇水");
#endif
          reply("OK none");
        } else {
          snprintf(msg, sizeof(msg), "排队 %u 盆", out.queued);
#if BOARD_HAS_STATUS_OLED
          status_oled_set_note(msg);
#endif
          snprintf(msg, sizeof(msg), "OK %u", out.queued);
          reply(msg);
        }
        return;
    }
  }
  long epoch = 0;
  if (sscanf(line, "time %ld", &epoch) == 1) {
    if (epoch < 1700000000L) {
      reply("ERR time");
      return;
    }
    struct timeval tv;
    tv.tv_sec = static_cast<time_t>(epoch);
    tv.tv_usec = 0;
    if (settimeofday(&tv, nullptr) != 0) {
      reply("ERR time");
      return;
    }
    struct tm ti;
    localtime_r(&tv.tv_sec, &ti);
    Serial.printf("TIME: %04d-%02d-%02d %02d:%02d\n", ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday,
                  ti.tm_hour, ti.tm_min);
    reply("OK");
    return;
  }
  if (sscanf(line, "water %d %d", &a, &b) == 2) {
    if (!zoneOk(a) || b <= 0) {
      reply("ERR zone");
      return;
    }
    const bool ok = g_controller.requestManual(static_cast<uint8_t>(a), static_cast<uint16_t>(b));
    reply(ok ? "OK" : "ERR busy");
    return;
  }
  if (sscanf(line, "auto %d %d", &a, &b) == 2) {
    if (!zoneOk(a)) {
      reply("ERR zone");
      return;
    }
    g_zone_configs[a].auto_enabled = b != 0;
    saveSettings();
    reply("OK");
    return;
  }
  if (sscanf(line, "vol %d %d", &a, &b) == 2) {
    if (!zoneOk(a) || b <= 0) {
      reply("ERR zone");
      return;
    }
    g_zone_configs[a].volume_ml = static_cast<uint16_t>(b);
    saveSettings();
    reply("OK");
    return;
  }
  int c = 0;
  if (sscanf(line, "th %d %d %d", &a, &b, &c) == 3) {
    if (!zoneOk(a) || b < 0 || c < 0 || b > 100 || c > 100 || b >= c) {
      reply("ERR zone");
      return;
    }
    g_zone_configs[a].moisture_low = static_cast<uint8_t>(b);
    g_zone_configs[a].moisture_high = static_cast<uint8_t>(c);
    saveSettings();
    reply("OK");
    return;
  }
  if (sscanf(line, "pump %d", &a) == 1) {
    g_pump.set(a != 0);
    reply("OK");
    return;
  }
  if (strncmp(line, "valve ", 6) == 0) {
#if !IRRIGATION_HAS_VALVES
    (void)a;
    reply("ERR valve");
    return;
#else
    if (strcmp(line + 6, "off") == 0) {
      g_valves.closeAll();
      reply("OK");
      return;
    }
    if (sscanf(line + 6, "%d", &a) == 1 && zoneOk(a)) {
      g_valves.open(static_cast<uint8_t>(a));
      reply("OK");
      return;
    }
    reply("ERR valve");
    return;
#endif
  }
  if (sscanf(line, "cal %d dry", &a) == 1 || sscanf(line, "cal %d wet", &a) == 1) {
    if (!zoneOk(a)) {
      reply("ERR zone");
      return;
    }
    const bool wet = strstr(line, "wet") != nullptr;
    g_zone_manager.sampleAll();
    if (wet) {
      g_zone_configs[a].cal_wet = g_zone_status[a].moisture_adc;
    } else {
      g_zone_configs[a].cal_dry = g_zone_status[a].moisture_adc;
    }
    saveSettings();
    reply("OK");
    return;
  }
  if (strcmp(line, "hist") == 0) {
    replyHistory();
    return;
  }
  if (strcmp(line, "wifi?") == 0) {
    replyWifi();
    return;
  }
  if (strncmp(line, "auto all ", 9) == 0) {
    const bool on = atoi(line + 9) != 0;
    for (uint8_t i = 0; i < g_sys_config.zone_count; ++i) {
      g_zone_configs[i].auto_enabled = on;
    }
    saveSettings();
    reply("OK");
    return;
  }
  if (sscanf(line, "sch %d off", &a) == 1) {
    if (!zoneOk(a)) {
      reply("ERR zone");
      return;
    }
    g_zone_configs[a].schedule_enabled = false;
    saveSettings();
    reply("OK");
    return;
  }
  if (sscanf(line, "sch %d %d %d", &a, &b, &c) == 3) {
    if (!zoneOk(a) || !hmOk(b, c)) {
      reply("ERR zone");
      return;
    }
    g_zone_configs[a].schedule_enabled = true;
    g_zone_configs[a].schedule_hour = static_cast<uint8_t>(b);
    g_zone_configs[a].schedule_minute = static_cast<uint8_t>(c);
    g_zone_configs[a].schedule_fired_today = false;
    saveSettings();
    reply("OK");
    return;
  }
  int en = 0;
  int sh = 0;
  int sm = 0;
  int eh = 0;
  int em = 0;
  if (sscanf(line, "win %d %d %d %d %d", &en, &sh, &sm, &eh, &em) == 5) {
    if (!hmOk(sh, sm) || !hmOk(eh, em)) {
      reply("ERR zone");
      return;
    }
    g_sys_config.auto_window_enabled = en != 0;
    g_sys_config.auto_win_sh = static_cast<uint8_t>(sh);
    g_sys_config.auto_win_sm = static_cast<uint8_t>(sm);
    g_sys_config.auto_win_eh = static_cast<uint8_t>(eh);
    g_sys_config.auto_win_em = static_cast<uint8_t>(em);
    saveSettings();
    reply("OK");
    return;
  }
  unsigned long lim = 0;
  if (sscanf(line, "limit %lu", &lim) == 1) {
    if (lim < 100 || lim > 200000UL) {
      reply("ERR zone");
      return;
    }
    g_sys_config.daily_limit_ml = static_cast<uint32_t>(lim);
    g_safety.setDailyLimit(g_sys_config.daily_limit_ml);
    saveSettings();
    reply("OK");
    return;
  }
  if (sscanf(line, "zones %d", &a) == 1) {
    if (a < 1 || a > MAX_ZONES) {
      reply("ERR zone");
      return;
    }
    g_web.applyZoneCount(static_cast<uint8_t>(a));
    saveSettings();
    reply("OK");
    return;
  }
  if (sscanf(line, "ppl %d", &a) == 1) {
    if (a < 1 || a > 20000) {
      reply("ERR zone");
      return;
    }
    g_sys_config.pulses_per_liter = static_cast<uint16_t>(a);
    saveSettings();
    reply("OK");
    return;
  }
  if (strcmp(line, "flow 0") == 0) {
    g_flow.resetSession();
    reply("OK");
    return;
  }
  if (sscanf(line, "purge %d", &a) == 1) {
    purgeSet(a != 0);
    reply("OK");
    return;
  }
  if (strncmp(line, "name ", 5) == 0) {
    int z = 0;
    if (sscanf(line + 5, "%d", &z) != 1 || !zoneOk(z)) {
      reply("ERR zone");
      return;
    }
    const char *sp = strchr(line + 5, ' ');
    if (sp == nullptr || sp[1] == '\0') {
      reply("ERR zone");
      return;
    }
    copyName(g_zone_configs[z].name, sizeof(g_zone_configs[z].name), sp + 1);
    saveSettings();
    reply("OK");
    return;
  }
  if (strncmp(line, "ssid ", 5) == 0) {
    copyName(g_sys_config.wifi_ssid, sizeof(g_sys_config.wifi_ssid), line + 5);
    saveSettings();
    reply("OK");
    return;
  }
  if (strncmp(line, "pass ", 5) == 0) {
    strncpy(g_sys_config.wifi_pass, line + 5, sizeof(g_sys_config.wifi_pass) - 1);
    g_sys_config.wifi_pass[sizeof(g_sys_config.wifi_pass) - 1] = '\0';
    saveSettings();
    reply("OK");
    return;
  }
  if (sscanf(line, "wifi %d", &a) == 1) {
    g_sys_config.wifi_enabled = a != 0;
    saveSettings();
    g_web.restartWiFi();
    reply("OK");
    return;
  }
  reply("ERR cmd");
}

class ServerCb : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    (void)server;
    g_connected = true;
  }
  void onDisconnect(BLEServer *server) override {
    (void)server;
    g_connected = false;
    BLEDevice::startAdvertising();
  }
};

class CmdCb : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *ch) override {
    std::string v = ch->getValue();
    if (v.empty()) {
      return;
    }
    portENTER_CRITICAL(&g_mux);
    if (!g_cmd_pending) {
      const size_t n = v.size() < sizeof(g_cmd) - 1 ? v.size() : sizeof(g_cmd) - 1;
      memcpy(g_cmd, v.data(), n);
      g_cmd[n] = '\0';
      g_cmd_pending = true;
    }
    portEXIT_CRITICAL(&g_mux);
  }
};

ServerCb g_server_cb;
CmdCb g_cmd_cb;

size_t buildStatus(uint8_t *out, size_t cap) {
  const uint8_t n =
      g_sys_config.zone_count > MAX_ZONES ? MAX_ZONES : g_sys_config.zone_count;
  const size_t need = 28 + static_cast<size_t>(n) * 24;
  if (cap < need) {
    return 0;
  }
  memset(out, 0, need);
  out[0] = 2;
  out[1] = static_cast<uint8_t>(g_sys_status.state);
  out[2] = static_cast<uint8_t>(g_sys_status.safety);
  uint8_t flags = 0;
  if (g_sys_status.pump_on) flags |= 0x01;
  if (g_sys_status.valve_on) flags |= 0x02;
  if (g_safety.isLocked()) flags |= 0x04;
  if (g_sys_status.purge_on) flags |= 0x08;
  if (WiFi.status() == WL_CONNECTED) flags |= 0x10;
  out[3] = flags;
  out[4] = g_sys_status.active_valve < 0 ? 255 : static_cast<uint8_t>(g_sys_status.active_valve);
  out[5] = g_sys_status.queue_len;
  out[6] = n;
  out[7] = g_sys_config.auto_window_enabled ? 1 : 0;
  const uint16_t daily =
      g_sys_status.daily_ml > 65535 ? 65535 : static_cast<uint16_t>(g_sys_status.daily_ml);
  out[8] = static_cast<uint8_t>(daily & 0xFF);
  out[9] = static_cast<uint8_t>(daily >> 8);
  out[10] = static_cast<uint8_t>(g_sys_status.session_ml & 0xFF);
  out[11] = static_cast<uint8_t>(g_sys_status.session_ml >> 8);
  const uint32_t limit = g_sys_config.daily_limit_ml;
  out[12] = static_cast<uint8_t>(limit & 0xFF);
  out[13] = static_cast<uint8_t>((limit >> 8) & 0xFF);
  out[14] = static_cast<uint8_t>((limit >> 16) & 0xFF);
  out[15] = static_cast<uint8_t>((limit >> 24) & 0xFF);
  out[16] = g_sys_config.auto_win_sh;
  out[17] = g_sys_config.auto_win_sm;
  out[18] = g_sys_config.auto_win_eh;
  out[19] = g_sys_config.auto_win_em;
  out[20] = static_cast<uint8_t>(g_sys_config.pulses_per_liter & 0xFF);
  out[21] = static_cast<uint8_t>(g_sys_config.pulses_per_liter >> 8);
  const uint16_t flowMl = g_flow.volumeMl(g_sys_config.pulses_per_liter);
  out[22] = static_cast<uint8_t>(flowMl & 0xFF);
  out[23] = static_cast<uint8_t>(flowMl >> 8);
  const uint32_t pulses = g_flow.pulses();
  out[24] = static_cast<uint8_t>(pulses & 0xFF);
  out[25] = static_cast<uint8_t>((pulses >> 8) & 0xFF);
  out[26] = static_cast<uint8_t>((pulses >> 16) & 0xFF);
  out[27] = static_cast<uint8_t>((pulses >> 24) & 0xFF);

  for (uint8_t i = 0; i < n; ++i) {
    uint8_t *z = out + 28 + static_cast<size_t>(i) * 24;
    const ZoneStatus &st = g_zone_status[i];
    const ZoneConfig &cfg = g_zone_configs[i];
    z[0] = st.moisture_pct;
    z[1] = cfg.moisture_low;
    z[2] = cfg.moisture_high;
    uint8_t zf = 0;
    if (cfg.auto_enabled) zf |= 0x01;
    if (st.sensor_valid) zf |= 0x02;
    if (cfg.schedule_enabled) zf |= 0x04;
    z[3] = zf;
    z[4] = static_cast<uint8_t>(cfg.volume_ml & 0xFF);
    z[5] = static_cast<uint8_t>(cfg.volume_ml >> 8);
    z[6] = static_cast<uint8_t>(st.moisture_adc & 0xFF);
    z[7] = static_cast<uint8_t>(st.moisture_adc >> 8);
    z[8] = cfg.schedule_hour;
    z[9] = cfg.schedule_minute;
    strncpy(reinterpret_cast<char *>(z + 10), cfg.name, 13);
    z[23] = 0;
  }
  return need;
}

void notifyStatus() {
  if (!g_connected || g_status == nullptr) {
    return;
  }
  uint8_t payload[28 + MAX_ZONES * 24];
  const size_t n = buildStatus(payload, sizeof(payload));
  if (n == 0) {
    return;
  }
  const uint8_t chunks = static_cast<uint8_t>((n + kChunkPayload - 1) / kChunkPayload);
  for (uint8_t i = 0; i < chunks; ++i) {
    uint8_t frame[3 + kChunkPayload];
    const size_t off = static_cast<size_t>(i) * kChunkPayload;
    const size_t len = (off + kChunkPayload <= n) ? kChunkPayload : (n - off);
    frame[0] = 0xA5;
    frame[1] = i;
    frame[2] = chunks;
    memcpy(frame + 3, payload + off, len);
    g_status->setValue(frame, 3 + len);
    g_status->notify();
    delay(15);
  }
}

void startBle() {
  BLEDevice::init("Langua");
  BLEDevice::setMTU(185);
  g_server = BLEDevice::createServer();
  g_server->setCallbacks(&g_server_cb);

  BLEService *svc = g_server->createService(kSvc);
  g_status = svc->createCharacteristic(kStatus, BLECharacteristic::PROPERTY_NOTIFY);
  g_status->addDescriptor(new BLE2902());
  g_reply = svc->createCharacteristic(kReply, BLECharacteristic::PROPERTY_NOTIFY);
  g_reply->addDescriptor(new BLE2902());
  BLECharacteristic *cmd =
      svc->createCharacteristic(kCmd, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  cmd->setCallbacks(&g_cmd_cb);
  svc->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(kSvc);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  BLEDevice::startAdvertising();
  g_inited = true;
  Serial.println(F("BLE: advertising as Langua"));
}

}  // namespace

void ble_link_begin() {
  if (g_inited) {
    return;
  }
  startBle();
}

void ble_link_loop() {
  if (!g_inited) {
    ble_link_begin();
  }

  if (g_cmd_pending) {
    char local[48];
    portENTER_CRITICAL(&g_mux);
    strncpy(local, g_cmd, sizeof(local) - 1);
    local[sizeof(local) - 1] = '\0';
    g_cmd_pending = false;
    portEXIT_CRITICAL(&g_mux);
    handleCommand(local);
  }

  const uint32_t now = millis();
  if (g_connected && now - g_last_notify_ms >= kNotifyMs) {
    g_last_notify_ms = now;
    notifyStatus();
  }
}
