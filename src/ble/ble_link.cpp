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

char g_cmd[48];
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

void handleCommand(char *line) {
  while (*line == ' ') {
    ++line;
  }
  if (!g_selfcheck.done && strcmp(line, "estop") != 0 && strcmp(line, "stop") != 0 &&
      strncmp(line, "time ", 5) != 0) {
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
  const size_t need = 12 + static_cast<size_t>(n) * 16;
  if (cap < need) {
    return 0;
  }
  out[0] = 1;
  out[1] = static_cast<uint8_t>(g_sys_status.state);
  out[2] = static_cast<uint8_t>(g_sys_status.safety);
  uint8_t flags = 0;
  if (g_sys_status.pump_on) flags |= 0x01;
  if (g_sys_status.valve_on) flags |= 0x02;
  if (g_safety.isLocked()) flags |= 0x04;
  out[3] = flags;
  out[4] = g_sys_status.active_valve < 0 ? 255 : static_cast<uint8_t>(g_sys_status.active_valve);
  out[5] = g_sys_status.queue_len;
  out[6] = n;
  out[7] = 0;
  const uint16_t daily =
      g_sys_status.daily_ml > 65535 ? 65535 : static_cast<uint16_t>(g_sys_status.daily_ml);
  out[8] = static_cast<uint8_t>(daily & 0xFF);
  out[9] = static_cast<uint8_t>(daily >> 8);
  out[10] = static_cast<uint8_t>(g_sys_status.session_ml & 0xFF);
  out[11] = static_cast<uint8_t>(g_sys_status.session_ml >> 8);

  for (uint8_t i = 0; i < n; ++i) {
    uint8_t *z = out + 12 + static_cast<size_t>(i) * 16;
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
    memset(z + 8, 0, 8);
    strncpy(reinterpret_cast<char *>(z + 8), cfg.name, 7);
    z[15] = 0;
  }
  return need;
}

void notifyStatus() {
  if (!g_connected || g_status == nullptr) {
    return;
  }
  uint8_t payload[12 + MAX_ZONES * 16];
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
