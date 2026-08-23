#include "ui/st7789_panel.h"

#if BOARD_HAS_LVGL

#include <Arduino.h>
#include <SPI.h>

#include "config.h"  // 链入 platform.h→board_s3.h→board_io_map.h（PIN_LCD_* 可见）

#include "boards/board_s3.h"

namespace {

SPIClass g_lcd_spi(FSPI);
constexpr uint32_t kSpiHz = 40000000;

void writeCommand(uint8_t cmd) {
  digitalWrite(PIN_LCD_DC, LOW);
  digitalWrite(PIN_LCD_CS, LOW);
  g_lcd_spi.transfer(cmd);
  digitalWrite(PIN_LCD_CS, HIGH);
}

void writeData8(uint8_t data) {
  digitalWrite(PIN_LCD_DC, HIGH);
  digitalWrite(PIN_LCD_CS, LOW);
  g_lcd_spi.transfer(data);
  digitalWrite(PIN_LCD_CS, HIGH);
}

void writeData16(uint16_t data) {
  digitalWrite(PIN_LCD_DC, HIGH);
  digitalWrite(PIN_LCD_CS, LOW);
  g_lcd_spi.transfer16(data);
  digitalWrite(PIN_LCD_CS, HIGH);
}

void setAddrWindow(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
  writeCommand(0x2A);
  writeData8(static_cast<uint8_t>(x1 >> 8));
  writeData8(static_cast<uint8_t>(x1 & 0xFF));
  writeData8(static_cast<uint8_t>(x2 >> 8));
  writeData8(static_cast<uint8_t>(x2 & 0xFF));

  writeCommand(0x2B);
  writeData8(static_cast<uint8_t>(y1 >> 8));
  writeData8(static_cast<uint8_t>(y1 & 0xFF));
  writeData8(static_cast<uint8_t>(y2 >> 8));
  writeData8(static_cast<uint8_t>(y2 & 0xFF));

  writeCommand(0x2C);
}

}  // namespace

bool st7789_panel_begin() {
  pinMode(PIN_LCD_DC, OUTPUT);
  pinMode(PIN_LCD_CS, OUTPUT);
  pinMode(PIN_LCD_RST, OUTPUT);
  pinMode(PIN_LCD_BLK, OUTPUT);

  digitalWrite(PIN_LCD_CS, HIGH);
  digitalWrite(PIN_LCD_DC, HIGH);

  g_lcd_spi.begin(PIN_LCD_SCLK, -1, PIN_LCD_MOSI, PIN_LCD_CS);

  digitalWrite(PIN_LCD_RST, HIGH);
  delay(10);
  digitalWrite(PIN_LCD_RST, LOW);
  delay(10);
  digitalWrite(PIN_LCD_RST, HIGH);
  delay(120);

  writeCommand(0x01);  // SW reset
  delay(150);

  writeCommand(0x11);  // Sleep out
  delay(120);

  writeCommand(0x3A);
  writeData8(0x55);  // 16-bit

  writeCommand(0x36);  // MADCTL — landscape 320x240
  writeData8(0x70);

  writeCommand(0x21);  // Inversion on (common for IPS 2.4")

  writeCommand(0x13);  // Normal display mode on

  writeCommand(0x29);  // Display on
  delay(20);

  st7789_panel_set_backlight(100);
  return true;
}

void st7789_panel_set_backlight(uint8_t pct) {
  if (pct >= 100) {
    digitalWrite(PIN_LCD_BLK, HIGH);
  } else if (pct == 0) {
    digitalWrite(PIN_LCD_BLK, LOW);
  } else {
    analogWrite(PIN_LCD_BLK, (pct * 255) / 100);
  }
}

void st7789_panel_draw_rgb565(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                              const uint16_t *pixels) {
  if (pixels == nullptr) {
    return;
  }

  const int32_t w = x2 - x1 + 1;
  const int32_t h = y2 - y1 + 1;
  const int32_t count = w * h;
  if (count <= 0) {
    return;
  }

  g_lcd_spi.beginTransaction(SPISettings(kSpiHz, MSBFIRST, SPI_MODE0));
  setAddrWindow(x1, y1, x2, y2);

  digitalWrite(PIN_LCD_DC, HIGH);
  digitalWrite(PIN_LCD_CS, LOW);
  for (int32_t i = 0; i < count; ++i) {
    g_lcd_spi.transfer16(pixels[i]);
  }
  digitalWrite(PIN_LCD_CS, HIGH);
  g_lcd_spi.endTransaction();
}

#endif
