#include <Arduino.h>

#include "actuator/pca9555.h"
#include "actuator/pump_driver.h"
#include "actuator/valve_driver.h"
#include "bus/i2c_bus.h"
#include "cli/serial_cli.h"
#include "config.h"
#include "control/irrigation_controller.h"
#include "control/stress_test.h"
#include "control/zone_manager.h"
#include "safety/safety_monitor.h"
#include "safety/selfcheck.h"
#include "sensor/ads1115.h"
#include "sensor/flow_meter.h"
#include "sensor/moisture_sensor.h"
#include "storage/irrigation_history.h"
#include "storage/settings_store.h"
#include "web/web_server.h"
#include "ble/ble_link.h"
#include "led/status_led.h"

#if BOARD_HAS_OLED
#include "display/display_driver.h"
#endif

#if BOARD_HAS_LVGL
#include "ui/lvgl_port.h"
#include "ui/app_ui.h"
#endif

#if BOARD_HAS_STATUS_OLED
#include "ui/panel_buttons.h"
#include "ui/status_oled.h"
#endif

#include <time.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

MoistureSensor g_moisture_sensors[MAX_ZONES] = {
    MoistureSensor(0), MoistureSensor(1), MoistureSensor(2), MoistureSensor(3),
    MoistureSensor(4), MoistureSensor(5), MoistureSensor(6), MoistureSensor(7),
    MoistureSensor(8), MoistureSensor(9),
};

FlowMeter g_flow(PIN_FLOW_METER);
PumpDriver g_pump;
ValveDriver g_valves;
SafetyMonitor g_safety;
SettingsStore g_settings;
SerialCli g_cli;
WebServerUi g_web;
#if BOARD_HAS_OLED
DisplayDriver g_display;
#endif

SystemConfig g_sys_config;
ZoneConfig g_zone_configs[MAX_ZONES];
ZoneStatus g_zone_status[MAX_ZONES];
SystemStatus g_sys_status;

ZoneManager g_zone_manager(g_moisture_sensors, g_zone_configs, g_zone_status, MAX_ZONES);
#if IRRIGATION_HAS_VALVES
ValveDriver *const g_valves_ptr = &g_valves;
#else
ValveDriver *const g_valves_ptr = nullptr;
#endif
#if IRRIGATION_HAS_FLOW_METER
FlowMeter *const g_flow_ptr = &g_flow;
#else
FlowMeter *const g_flow_ptr = nullptr;
#endif
IrrigationController g_controller(&g_zone_manager, &g_pump, g_valves_ptr, g_flow_ptr, &g_safety,
                                  &g_sys_config, g_zone_configs, g_zone_status, &g_sys_status);
SystemContext g_ctx;
SystemContextEx g_ctx_ex;

uint32_t last_sample_ms = 0;
uint32_t last_status_ms = 0;
uint32_t last_tick_ms = 0;
uint32_t last_led_ms = 0;
#if BOARD_HAS_OLED || BOARD_HAS_STATUS_OLED
uint32_t last_display_ms = 0;
#endif

void clampZoneCount() {
  if (g_sys_config.zone_count < MIN_ZONE_COUNT) {
    g_sys_config.zone_count = MIN_ZONE_COUNT;
  }
  if (g_sys_config.zone_count > MAX_ZONES) {
    g_sys_config.zone_count = MAX_ZONES;
  }
#if defined(BOARD_VALVE_COUNT)
  if (g_sys_config.zone_count > BOARD_VALVE_COUNT) {
    g_sys_config.zone_count = BOARD_VALVE_COUNT;
  }
#endif
  g_zone_manager.setCount(g_sys_config.zone_count);
}

// 排气模式：全部阀打开 + 泵直通（绕过控制器 → 不触发干转判定），手动停。
// 用于新装/久置后排出管内空气；先清故障锁定与队列，避免与自动浇水冲突。
void purgeSet(bool on) {
  if (on) {
    g_controller.stop();  // 清故障/队列/解锁
    for (uint8_t i = 0; i < g_sys_config.zone_count; ++i) {
      g_pca9555.setDriveLow(kValvePcaBit[i]);  // 全开（0V → 低电平触发吸合）
    }
    g_pump.set(true);
  } else {
    g_pump.set(false);
    g_valves.closeAll();  // 全关（高阻）
  }
  g_sys_status.purge_on = on;
  g_sys_status.pump_on = g_pump.isOn();
  g_sys_status.valve_on = on;  // 排气时视为阀全开
  Serial.printf("purge %s\n", on ? "ON" : "OFF");
}

void updateStatusLed() {
  const uint32_t now = millis();
  if (now - last_led_ms < LED_UPDATE_MS) {
    return;
  }
  last_led_ms = now;

  static bool led_on = false;

  // 板载 WS2812 RGB 状态灯：红=故障/锁定、蓝闪=泵阀工作、绿=空闲在线
  if (g_sys_status.state == IrrigationState::Fault || g_safety.isLocked()) {
    statusLedSet(0xFF0000);
    return;
  }
  if (g_sys_status.state == IrrigationState::Pumping ||
      g_sys_status.state == IrrigationState::Valving) {
    led_on = !led_on;
    statusLedSet(led_on ? 0x0000FF : 0x000000);
    return;
  }
  statusLedSet(0x00FF00);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  statusLedBegin();

  // 硬件看门狗：loop 卡死自动复位（safety-checklist §4）
  esp_task_wdt_init(10, true);
  esp_task_wdt_add(nullptr);  // 注册 Arduino loopTask

  Serial.printf("AutoIrrigation v%s\n", FIRMWARE_VERSION);
  Serial.printf("Board: %s\n", BOARD_NAME);

  irrigationI2cBegin();

  // 开机自检（I2C 设备清单）；done=false 期间 Web/CLI 写操作锁定
  run_selfcheck();

  // 设备 begin 复用 selfcheck 探测结果：无设备时不重复探测
  // （ESP32 Wire 无 ACK 探测实测固定 ~1s/次，重复探测会拖慢启动）
  if (g_selfcheck.ads_ok > 0) {
    g_ads1115.begin();
  } else {
    Serial.println(F("WARN: no ADS1115 detected on I2C"));
  }
#if IRRIGATION_HAS_VALVES
  if (g_selfcheck.pca9555_ok) {
    g_pca9555.begin(PCA9555_ADDR_VALVES);
    g_valves.begin(BOARD_VALVE_COUNT, VALVE_ACTIVE_HIGH != 0);
  } else {
    Serial.println(F("WARN: PCA9555 valve expander not found"));
  }
#else
  Serial.println(F("INFO: valves disabled (no PCA9555 / solenoids)"));
#endif

#if BOARD_HAS_OLED
  delay(50);
  if (g_display.begin()) {
    Serial.println(F("OLED: U8g2 72x40 initialized"));
    g_display.showSplash();
  } else {
    Serial.println(F("WARN: OLED init failed"));
    irrigationI2cScan();
  }
#endif

  g_pump.begin(PIN_PUMP_RELAY, PUMP_ACTIVE_HIGH != 0);
#if IRRIGATION_HAS_FLOW_METER
  g_flow.begin();
#else
  Serial.println(F("INFO: flow meter disabled (time-based volume estimate)"));
#endif
  g_zone_manager.begin();

  g_settings.begin();
  g_history.begin();
  g_ctx = {&g_sys_config, g_zone_configs, g_zone_status, &g_sys_status};
  g_ctx_ex = {&g_sys_config, g_zone_configs, g_zone_status, &g_sys_status, &g_controller};
  g_settings.applyDefaults(g_ctx);
  if (!g_settings.load(g_ctx)) {
    Serial.println(F("NVS: using defaults"));
  }
  clampZoneCount();

#if !IRRIGATION_HAS_FLOW_METER
  g_sys_config.dry_run_sec = 0;
#endif

  g_safety.begin(g_sys_config.daily_limit_ml, g_sys_config.max_run_sec, g_sys_config.dry_run_sec);
  g_controller.begin();
  g_cli.begin(&g_ctx_ex);
  g_web.begin(&g_ctx_ex);
  g_stress.begin(&g_ctx_ex);
#if BOARD_HAS_LVGL
  if (lvgl_port_begin()) {
    app_ui_begin(&g_ctx_ex);
    Serial.println(F("LVGL: ST7789 initialized"));
  } else {
    Serial.println(F("WARN: LVGL display init failed"));
  }
#endif

#if BOARD_HAS_STATUS_OLED
  if (status_oled_begin()) {
    Serial.println(F("OLED: SSD1306 128x64 initialized"));
  } else {
    Serial.println(F("WARN: status OLED init failed"));
  }
  panel_buttons_begin();
#endif

  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.printf("Zones: %u (max %u valves, I2C expanders)\n", g_sys_config.zone_count,
                BOARD_VALVE_COUNT);
#if !IRRIGATION_HAS_FLOW_METER || !IRRIGATION_HAS_VALVES
  Serial.print(F("Bring-up: "));
#if !IRRIGATION_HAS_VALVES
  Serial.print(F("no valves "));
#endif
#if !IRRIGATION_HAS_FLOW_METER
  Serial.printf("no flow (~%u ml/s est) ", DEFAULT_PUMP_FLOW_ML_PER_SEC);
#endif
  Serial.println();
#endif
  Serial.printf("WiFi: %s\n", g_sys_config.wifi_enabled ? "enabled" : "disabled");
  Serial.println(F("Ready. Type help."));
}

void loop() {
  esp_task_wdt_reset();

  const uint32_t now = millis();

  g_cli.poll();
  g_web.loop();
  ble_link_loop();

  if (now - last_sample_ms >= SAMPLE_INTERVAL_MS) {
    last_sample_ms = now;
    g_zone_manager.sampleAll();
  }

  if (now - last_tick_ms >= CONTROLLER_TICK_MS) {
    last_tick_ms = now;
    g_safety.setDailyLimit(g_sys_config.daily_limit_ml);
    g_safety.setMaxRunSec(g_sys_config.max_run_sec);
#if IRRIGATION_HAS_FLOW_METER
    g_safety.setDryRunSec(g_sys_config.dry_run_sec);
#else
    g_safety.setDryRunSec(0);
#endif
    g_controller.tick();
    g_stress.tick();
  }

  if (now - last_status_ms >= STATUS_PRINT_INTERVAL_MS) {
    last_status_ms = now;
    g_cli.printStatus();
  }

#if BOARD_HAS_STATUS_OLED
  panel_buttons_poll();
  if (now - last_display_ms >= DISPLAY_INTERVAL_MS) {
    last_display_ms = now;
    status_oled_show(g_sys_status, g_zone_status, g_zone_configs, g_sys_config.zone_count,
                     g_controller.stateText());
  }
#endif

#if BOARD_HAS_OLED
  if (now - last_display_ms >= DISPLAY_INTERVAL_MS) {
    last_display_ms = now;
    const bool ap_on =
        WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA;
    g_display.showStatus(g_sys_status, g_zone_status, g_sys_config.zone_count, &g_controller,
                         ap_on);
  }
#endif

#if BOARD_HAS_LVGL
  lvgl_port_loop();
  app_ui_update();
#endif

  updateStatusLed();
}
