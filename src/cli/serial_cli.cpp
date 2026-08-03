#include "cli/serial_cli.h"

#include <Arduino.h>

#include "actuator/pump_driver.h"
#include "actuator/valve_driver.h"
#include "config.h"
#include "control/irrigation_controller.h"
#include "control/zone_manager.h"
#include "safety/safety_monitor.h"
#include "bus/i2c_bus.h"
#include "sensor/flow_meter.h"
#include "storage/settings_store.h"

extern PumpDriver g_pump;
extern ValveDriver g_valves;
extern FlowMeter g_flow;
extern ZoneManager g_zone_manager;
extern SettingsStore g_settings;

void SerialCli::begin(SystemContextEx *ctx) {
  ctx_ = ctx;
  line_buf_.reserve(128);
}

void SerialCli::printHelp() const {
  Serial.println(F("Commands:"));
  Serial.println(F("  help              - list commands"));
  Serial.println(F("  status            - system status"));
  Serial.println(F("  water <z> <ml>    - manual irrigate"));
  Serial.println(F("  stop              - stop pump, clear fault"));
  Serial.println(F("  auto on|off [z]   - threshold mode"));
  Serial.println(F("  set low|high|vol <z> <val>"));
  Serial.println(F("  set schedule <z> HH:MM"));
  Serial.println(F("  schedule <z> on|off"));
  Serial.println(F("  cal dry|wet <z>   - moisture calibration"));
  Serial.println(F("  flow              - pulse count"));
  Serial.println(F("  pump on|off       - debug pump"));
  Serial.println(F("  valve <z>|off     - debug valve"));
  Serial.println(F("  set zones <n>     - active zone count (1-10)"));
  Serial.println(F("  ppl <n>           - pulses per liter"));
  Serial.println(F("  wifi ssid|pass|on|off"));
  Serial.println(F("  save              - save to NVS"));
  Serial.println(F("  i2cscan           - scan I2C bus"));
}

void SerialCli::printStatus() const {
  if (ctx_ == nullptr || ctx_->status == nullptr || ctx_->config == nullptr) {
    return;
  }
  Serial.print(F("[status] pump="));
  Serial.print(ctx_->status->pump_on ? F("ON") : F("OFF"));
  Serial.print(F(" valve="));
  if (ctx_->status->valve_on && ctx_->status->active_valve >= 0) {
    Serial.print(F("Z"));
    Serial.print(ctx_->status->active_valve);
  } else {
    Serial.print(F("OFF"));
  }
  Serial.print(F(" state="));
  if (ctx_->controller != nullptr) {
    Serial.print(ctx_->controller->stateText());
  }
  Serial.print(F(" safety="));
  Serial.print(ctx_->status->safety == SafetyState::Ok ? F("OK") : F("FAULT"));
  Serial.print(F(" daily="));
  Serial.print(ctx_->status->daily_ml);
  Serial.print(F("ml queue="));
  Serial.print(ctx_->status->queue_len);

  for (uint8_t i = 0; i < ctx_->config->zone_count; ++i) {
    Serial.print(F(" Z"));
    Serial.print(i);
    Serial.print(F("="));
    if (ctx_->zone_status != nullptr) {
      Serial.print(ctx_->zone_status[i].moisture_pct);
    } else {
      Serial.print(F("?"));
    }
    Serial.print(F("%"));
  }
  Serial.println();
}

void SerialCli::dispatch(const char *line) {
  if (ctx_ == nullptr) {
    return;
  }

  char buf[128];
  strncpy(buf, line, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  char *cmd = strtok(buf, " ");
  if (cmd == nullptr) {
    return;
  }

  if (strcmp(cmd, "help") == 0) {
    printHelp();
  } else if (strcmp(cmd, "status") == 0) {
    printStatus();
  } else if (strcmp(cmd, "water") == 0) {
    char *z = strtok(nullptr, " ");
    char *ml = strtok(nullptr, " ");
    if (z && ml && ctx_->controller) {
      ctx_->controller->requestManual(static_cast<uint8_t>(atoi(z)),
                                      static_cast<uint16_t>(atoi(ml)));
      Serial.println(F("Queued."));
    }
  } else if (strcmp(cmd, "stop") == 0) {
    if (ctx_->controller) {
      ctx_->controller->stop();
    }
    Serial.println(F("Stopped."));
  } else if (strcmp(cmd, "auto") == 0) {
    char *onoff = strtok(nullptr, " ");
    char *z = strtok(nullptr, " ");
    if (onoff && ctx_->zones) {
      const bool en = strcmp(onoff, "on") == 0;
      if (z) {
        const uint8_t zi = static_cast<uint8_t>(atoi(z));
        if (zi < ctx_->config->zone_count) {
          ctx_->zones[zi].auto_enabled = en;
        }
      } else {
        for (uint8_t i = 0; i < ctx_->config->zone_count; ++i) {
          ctx_->zones[i].auto_enabled = en;
        }
      }
      Serial.println(F("OK"));
    }
  } else if (strcmp(cmd, "set") == 0) {
    char *what = strtok(nullptr, " ");
    char *z = strtok(nullptr, " ");
    char *val = strtok(nullptr, " ");
    if (what && ctx_->zones) {
      if (strcmp(what, "zones") == 0) {
        if (!z) {
          return;
        }
        uint8_t n = static_cast<uint8_t>(atoi(z));
        if (n < MIN_ZONE_COUNT) {
          n = MIN_ZONE_COUNT;
        }
        if (n > MAX_ZONES) {
          n = MAX_ZONES;
        }
#if defined(BOARD_VALVE_COUNT)
        if (n > BOARD_VALVE_COUNT) {
          n = BOARD_VALVE_COUNT;
        }
#endif
        ctx_->config->zone_count = n;
        g_zone_manager.setCount(n);
        Serial.printf("OK zones=%u\n", n);
        return;
      }
      if (!z || !val) {
        return;
      }
      const uint8_t zi = static_cast<uint8_t>(atoi(z));
      if (zi >= ctx_->config->zone_count) {
        return;
      }
      if (strcmp(what, "low") == 0) {
        ctx_->zones[zi].moisture_low = static_cast<uint8_t>(atoi(val));
      } else if (strcmp(what, "high") == 0) {
        ctx_->zones[zi].moisture_high = static_cast<uint8_t>(atoi(val));
      } else if (strcmp(what, "vol") == 0) {
        ctx_->zones[zi].volume_ml = static_cast<uint16_t>(atoi(val));
      } else if (strcmp(what, "schedule") == 0) {
        int hh = 0, mm = 0;
        if (sscanf(val, "%d:%d", &hh, &mm) == 2) {
          ctx_->zones[zi].schedule_hour = static_cast<uint8_t>(hh);
          ctx_->zones[zi].schedule_minute = static_cast<uint8_t>(mm);
        }
      }
      Serial.println(F("OK"));
    }
  } else if (strcmp(cmd, "valve") == 0) {
#if !IRRIGATION_HAS_VALVES
    Serial.println(F("Valves disabled in this build."));
#else
    char *arg = strtok(nullptr, " ");
    if (arg) {
      if (strcmp(arg, "off") == 0) {
        g_valves.closeAll();
        Serial.println(F("OK"));
      } else {
        const uint8_t zi = static_cast<uint8_t>(atoi(arg));
        if (zi < ctx_->config->zone_count) {
          g_valves.open(zi);
          Serial.println(F("OK"));
        }
      }
    }
#endif
  } else if (strcmp(cmd, "schedule") == 0) {
    char *z = strtok(nullptr, " ");
    char *onoff = strtok(nullptr, " ");
    if (z && onoff && ctx_->zones) {
      const uint8_t zi = static_cast<uint8_t>(atoi(z));
      if (zi < ctx_->config->zone_count) {
        ctx_->zones[zi].schedule_enabled = strcmp(onoff, "on") == 0;
        Serial.println(F("OK"));
      }
    }
  } else if (strcmp(cmd, "cal") == 0) {
    char *kind = strtok(nullptr, " ");
    char *z = strtok(nullptr, " ");
    if (kind && z && ctx_->zones && ctx_->zone_status) {
      const uint8_t zi = static_cast<uint8_t>(atoi(z));
      if (zi < ctx_->config->zone_count) {
        const uint16_t adc = ctx_->zone_status[zi].moisture_adc;
        if (strcmp(kind, "dry") == 0) {
          ctx_->zones[zi].cal_dry = adc;
        } else if (strcmp(kind, "wet") == 0) {
          ctx_->zones[zi].cal_wet = adc;
        }
        Serial.printf("Zone %u cal %s = %u\n", zi, kind, adc);
      }
    }
  } else if (strcmp(cmd, "flow") == 0) {
#if !IRRIGATION_HAS_FLOW_METER
    Serial.println(F("Flow meter disabled in this build."));
#else
    Serial.printf("pulses=%lu vol=%u ml (ppl=%u)\n", g_flow.pulses(),
                  g_flow.volumeMl(ctx_->config->pulses_per_liter),
                  ctx_->config->pulses_per_liter);
#endif
  } else if (strcmp(cmd, "pump") == 0) {
    char *onoff = strtok(nullptr, " ");
    if (onoff) {
      g_pump.set(strcmp(onoff, "on") == 0);
      Serial.println(F("OK"));
    }
  } else if (strcmp(cmd, "ppl") == 0) {
    char *n = strtok(nullptr, " ");
    if (n && ctx_->config) {
      ctx_->config->pulses_per_liter = static_cast<uint16_t>(atoi(n));
      Serial.println(F("OK"));
    }
  } else if (strcmp(cmd, "wifi") == 0) {
    char *sub = strtok(nullptr, " ");
    if (sub && ctx_->config) {
      if (strcmp(sub, "on") == 0) {
        ctx_->config->wifi_enabled = true;
      } else if (strcmp(sub, "off") == 0) {
        ctx_->config->wifi_enabled = false;
      } else if (strcmp(sub, "ssid") == 0) {
        char *s = strtok(nullptr, " ");
        if (s) {
          strncpy(ctx_->config->wifi_ssid, s, sizeof(ctx_->config->wifi_ssid) - 1);
        }
      } else if (strcmp(sub, "pass") == 0) {
        char *p = strtok(nullptr, " ");
        if (p) {
          strncpy(ctx_->config->wifi_pass, p, sizeof(ctx_->config->wifi_pass) - 1);
        }
      }
      Serial.println(F("OK (reboot or save+restart WiFi service)"));
    }
  } else if (strcmp(cmd, "save") == 0) {
    SystemContext sc = {ctx_->config, ctx_->zones, ctx_->zone_status, ctx_->status};
    if (g_settings.save(sc)) {
      Serial.println(F("Saved."));
    } else {
      Serial.println(F("Save failed."));
    }
  } else if (strcmp(cmd, "i2cscan") == 0) {
    irrigationI2cScan();
  } else {
    Serial.println(F("Unknown command. Type help."));
  }
}

void SerialCli::poll() {
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      if (line_buf_.length() > 0) {
        dispatch(line_buf_.c_str());
        line_buf_ = "";
      }
    } else {
      line_buf_ += c;
      if (line_buf_.length() >= 120) {
        line_buf_ = "";
      }
    }
  }
}
