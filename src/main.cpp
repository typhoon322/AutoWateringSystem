#include <Arduino.h>

#include "actuator/pump_driver.h"
#include "cli/serial_cli.h"
#include "config.h"
#include "control/irrigation_controller.h"
#include "control/zone_manager.h"
#include "safety/safety_monitor.h"
#include "sensor/flow_meter.h"
#include "sensor/moisture_sensor.h"
#include "storage/settings_store.h"
#include "web/web_server.h"

#include <time.h>

MoistureSensor g_moisture_sensors[ACTIVE_ZONES] = {
    MoistureSensor(PIN_MOISTURE_Z0),
    MoistureSensor(PIN_MOISTURE_Z1),
};

FlowMeter g_flow(PIN_FLOW_METER);
PumpDriver g_pump;
SafetyMonitor g_safety;
SettingsStore g_settings;
SerialCli g_cli;
WebServerUi g_web;

SystemConfig g_sys_config;
ZoneConfig g_zone_configs[MAX_ZONES];
ZoneStatus g_zone_status[MAX_ZONES];
SystemStatus g_sys_status;

ZoneManager g_zone_manager(g_moisture_sensors, g_zone_configs, g_zone_status, ACTIVE_ZONES);
IrrigationController g_controller(&g_zone_manager, &g_pump, &g_flow, &g_safety, &g_sys_config,
                                  g_zone_configs, g_zone_status, &g_sys_status);
SystemContext g_ctx;
SystemContextEx g_ctx_ex;

uint32_t last_sample_ms = 0;
uint32_t last_status_ms = 0;
uint32_t last_tick_ms = 0;
uint32_t last_led_ms = 0;

void updateRgbLed() {
  const uint32_t now = millis();
  if (now - last_led_ms < LED_UPDATE_MS) {
    return;
  }
  last_led_ms = now;

  static bool led_state = false;
  pinMode(PIN_RGB_LED, OUTPUT);

  if (g_sys_status.state == IrrigationState::Fault || g_safety.isLocked()) {
    digitalWrite(PIN_RGB_LED, HIGH);
    return;
  }
  if (g_sys_status.state == IrrigationState::Pumping) {
    led_state = !led_state;
    digitalWrite(PIN_RGB_LED, led_state ? HIGH : LOW);
    return;
  }
  digitalWrite(PIN_RGB_LED, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.printf("AutoIrrigation v%s\n", FIRMWARE_VERSION);
  Serial.printf("Board: %s\n", BOARD_NAME);

  g_pump.begin(PIN_PUMP_RELAY, PUMP_ACTIVE_HIGH != 0);
  g_flow.begin();
  g_zone_manager.begin();

  g_settings.begin();
  g_ctx = {&g_sys_config, g_zone_configs, g_zone_status, &g_sys_status};
  g_ctx_ex = {&g_sys_config, g_zone_configs, g_zone_status, &g_sys_status, &g_controller};
  g_settings.applyDefaults(g_ctx);
  if (!g_settings.load(g_ctx)) {
    Serial.println(F("NVS: using defaults"));
  }

  g_safety.begin(g_sys_config.daily_limit_ml, g_sys_config.max_run_sec, g_sys_config.dry_run_sec);
  g_controller.begin();
  g_cli.begin(&g_ctx_ex);
  g_web.begin(&g_ctx_ex);

  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.printf("WiFi: %s\n", g_sys_config.wifi_enabled ? "enabled" : "disabled");
  Serial.println(F("Ready. Type help."));
}

void loop() {
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
    g_safety.setDryRunSec(g_sys_config.dry_run_sec);
    g_controller.tick();
  }

  if (now - last_status_ms >= STATUS_PRINT_INTERVAL_MS) {
    last_status_ms = now;
    g_cli.printStatus();
  }

  updateRgbLed();
}
