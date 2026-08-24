#include <Arduino.h>

#include "actuator/pca9555.h"
#include "actuator/pump_driver.h"
#include "actuator/valve_driver.h"
#include "bus/i2c_bus.h"
#include "cli/serial_cli.h"
#include "config.h"
#include "control/irrigation_controller.h"
#include "control/zone_manager.h"
#include "safety/safety_monitor.h"
#include "sensor/ads1115.h"
#include "sensor/flow_meter.h"
#include "sensor/moisture_sensor.h"
#include "storage/irrigation_history.h"
#include "storage/settings_store.h"
#include "web/web_server.h"

#if BOARD_HAS_OLED
#include "display/display_driver.h"
#endif

#if BOARD_HAS_LVGL
#include "ui/lvgl_port.h"
#include "ui/app_ui.h"
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
#if BOARD_HAS_OLED
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

void updateStatusLed() {
  const uint32_t now = millis();
  if (now - last_led_ms < LED_UPDATE_MS) {
    return;
  }
  last_led_ms = now;

  static bool led_on = false;
  pinMode(PIN_STATUS_LED, OUTPUT);

  const auto setLed = [](bool on) {
#if BOARD_LED_ACTIVE_LOW
    digitalWrite(PIN_STATUS_LED, on ? LOW : HIGH);
#else
    digitalWrite(PIN_STATUS_LED, on ? HIGH : LOW);
#endif
  };

  if (g_sys_status.state == IrrigationState::Fault || g_safety.isLocked()) {
    setLed(true);
    return;
  }
  if (g_sys_status.state == IrrigationState::Pumping ||
      g_sys_status.state == IrrigationState::Valving) {
    led_on = !led_on;
    setLed(led_on);
    return;
  }
  setLed(false);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // 硬件看门狗：loop 卡死 10s 自动复位（safety-checklist §4）
  esp_task_wdt_init(10, true);
  esp_task_wdt_add(nullptr);  // 注册 Arduino loopTask

  Serial.printf("AutoIrrigation v%s\n", FIRMWARE_VERSION);
  Serial.printf("Board: %s\n", BOARD_NAME);

  irrigationI2cBegin();

  if (!g_ads1115.begin()) {
    Serial.println(F("WARN: no ADS1115 detected on I2C"));
  } else {
    Serial.println(F("I2C: ADS1115 OK"));
  }
#if IRRIGATION_HAS_VALVES
  if (!g_pca9555.begin(PCA9555_ADDR_VALVES)) {
    Serial.println(F("WARN: PCA9555 valve expander not found"));
  } else {
    Serial.println(F("I2C: PCA9555 OK"));
  }
  g_valves.begin(BOARD_VALVE_COUNT, VALVE_ACTIVE_HIGH != 0);
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
#if BOARD_HAS_LVGL
  if (lvgl_port_begin()) {
    app_ui_begin(&g_ctx_ex);
    Serial.println(F("LVGL: ST7789 initialized"));
  } else {
    Serial.println(F("WARN: LVGL display init failed"));
  }
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
  }

  if (now - last_status_ms >= STATUS_PRINT_INTERVAL_MS) {
    last_status_ms = now;
    g_cli.printStatus();
  }

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
