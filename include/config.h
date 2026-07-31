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
#define DEFAULT_PULSES_PER_LITER 450
#define DEFAULT_DRY_RUN_SEC 3
#define DEFAULT_VALVE_SETTLE_MS 500

// ADC calibration defaults (12-bit)
#define DEFAULT_CAL_DRY 3200
#define DEFAULT_CAL_WET 1400

// Timing (ms)
#define SAMPLE_INTERVAL_MS 5000
#define STATUS_PRINT_INTERVAL_MS 1000
#define CONTROLLER_TICK_MS 100
#define LED_UPDATE_MS 500

// WiFi defaults (NVS overrides; WiFi off by default)
#define RADIO_WIFI_DEFAULT_ENABLED 0
#define WIFI_AP_FALLBACK 1
#define WIFI_AP_SSID "ESP32-IRRIGATION"
#define WIFI_AP_PASS "irrigate2026"
#define WIFI_RECONNECT_INTERVAL_MS 30000

// OTA
#define OTA_PASSWORD ""

// NVS
#define NVS_MAGIC 0x49525247U  // "IRRG"

// Safety ADC bounds
#define ADC_MIN_VALID 0
#define ADC_MAX_VALID 4095
