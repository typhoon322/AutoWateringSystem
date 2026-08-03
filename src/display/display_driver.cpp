#include "display/display_driver.h"
#include "config.h"

#if BOARD_HAS_OLED

#include <U8g2lib.h>
#include <Wire.h>

#include "bus/i2c_bus.h"
#include "control/irrigation_controller.h"

namespace {
U8G2_SSD1306_72X40_ER_F_HW_I2C g_oled(U8G2_R0, U8X8_PIN_NONE);

constexpr uint8_t kLineHeight = 10;

void printLine(uint8_t line, const char *text) {
  g_oled.drawStr(0, static_cast<int>(line + 1) * kLineHeight, text);
}
}  // namespace

bool DisplayDriver::begin() {
  irrigationI2cBegin();
  delay(20);

  bool oled_present = false;
  for (uint8_t attempt = 0; attempt < 5; ++attempt) {
    if (irrigationI2cProbe(OLED_I2C_ADDR)) {
      oled_present = true;
      break;
    }
    delay(20);
  }
  if (!oled_present) {
    initialized_ = false;
    return false;
  }

  g_oled.begin();
  g_oled.setBusClock(400000);
  g_oled.setContrast(255);
  g_oled.clearBuffer();
  g_oled.sendBuffer();
  initialized_ = true;
  return true;
}

void DisplayDriver::showSplash() {
  if (!initialized_) {
    return;
  }
  char line[16];
  g_oled.clearBuffer();
  g_oled.setFont(u8g2_font_6x10_tr);
  printLine(0, "AutoIrrigation");
  snprintf(line, sizeof(line), "v%s", FIRMWARE_VERSION);
  printLine(1, line);
  printLine(2, "Starting...");
  g_oled.sendBuffer();
}

void DisplayDriver::showStatus(const SystemStatus &status, const ZoneStatus *zones,
                               uint8_t zone_count, IrrigationController *controller,
                               bool wifi_ap_active) {
  if (!initialized_) {
    return;
  }

  char line[16];

  g_oled.clearBuffer();
  g_oled.setFont(u8g2_font_6x10_tr);

  snprintf(line, sizeof(line), "%s", controller != nullptr ? controller->stateText() : "IDLE");
  printLine(0, line);

  if (zone_count >= 2) {
    snprintf(line, sizeof(line), "Z0:%u%% Z1:%u%%", zones[0].moisture_pct, zones[1].moisture_pct);
  } else if (zone_count >= 1) {
    snprintf(line, sizeof(line), "Z0:%u%%", zones[0].moisture_pct);
  } else {
    snprintf(line, sizeof(line), "No zones");
  }
  printLine(1, line);

  snprintf(line, sizeof(line), "P:%s V:%s", status.pump_on ? "ON" : "OFF",
           status.valve_on ? "ON" : "OFF");
  printLine(2, line);

  snprintf(line, sizeof(line), "%luml %s", static_cast<unsigned long>(status.daily_ml),
           wifi_ap_active ? "AP" : "  ");
  printLine(3, line);

  g_oled.sendBuffer();
}

#else

bool DisplayDriver::begin() { return false; }

void DisplayDriver::showSplash() {}

void DisplayDriver::showStatus(const SystemStatus &, const ZoneStatus *, uint8_t,
                               IrrigationController *, bool) {}

#endif
