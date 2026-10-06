#pragma once

// Zone limits must be defined before platform.h (board_io_map.h uses MAX_ZONES)
#define MAX_ZONES 10

#include "platform.h"

#define FIRMWARE_VERSION "2.1"
#define ACTIVE_ZONES 2
#define MIN_ZONE_COUNT 1

// Moisture defaults (%)
#define DEFAULT_MOISTURE_LOW 30
#define DEFAULT_MOISTURE_HIGH 60

// Irrigation defaults
#define DEFAULT_VOLUME_ML 100
#define DEFAULT_MAX_RUN_SEC 60
#define DEFAULT_DAILY_LIMIT_ML 2000U
#define DEFAULT_PULSES_PER_LITER 5160  // OKD-HZ06D-1 霍尔流量计：官方表 K=86Hz/L/min → 5160 脉冲/升（量杯标定后以实测为准）
#define DEFAULT_DRY_RUN_SEC 3
#define DEFAULT_VALVE_SETTLE_MS 500

// Hardware present (override in platformio build_flags for bring-up)
#ifndef IRRIGATION_HAS_FLOW_METER
#define IRRIGATION_HAS_FLOW_METER 1
#endif
#ifndef IRRIGATION_HAS_VALVES
#define IRRIGATION_HAS_VALVES 1
#endif
// When flow meter absent: estimate volume from pump run time (ml/s, tune empirically)
#define DEFAULT_PUMP_FLOW_ML_PER_SEC 10

// ADC calibration defaults (12-bit)
#define DEFAULT_CAL_DRY 3200
#define DEFAULT_CAL_WET 1400

// Timing (ms)
#define SAMPLE_INTERVAL_MS 5000
// 浇完后先不看湿度：探头旁边会立刻被打湿，整盆还没渗匀。
#define MOISTURE_SOAK_MS (10UL * 60UL * 1000UL)
#define AUTO_MAX_DOSES 3
#define AUTO_DOSE_LOCKOUT_MS (60UL * 60UL * 1000UL)
#define STATUS_PRINT_INTERVAL_MS 1000
#define CONTROLLER_TICK_MS 100
#define LED_UPDATE_MS 500
#define DISPLAY_INTERVAL_MS 1000

// WiFi — 与 TempControl 相同；NVS 可覆盖 SSID/密码
#define RADIO_WIFI_DEFAULT_ENABLED 1
#define WIFI_SSID "OneMore"
#define WIFI_PASS "onemore.2025"
#define WIFI_HYBRID_MODE 1          // 1=AP+STA 同时开（混合模式）
#define WIFI_AP_FALLBACK 1
#define WIFI_AP_SSID "蓝瓜智控"      // UTF-8 中文 SSID（ESP32 原生支持）
#define WIFI_AP_PASS "langua123"
#define WIFI_RECONNECT_INTERVAL_MS 30000

// OTA
#define OTA_PASSWORD ""

// NVS
#define NVS_MAGIC 0x49525247U  // "IRRG"

// Safety ADC bounds
#define ADC_MIN_VALID 0
#define ADC_MAX_VALID 4095
