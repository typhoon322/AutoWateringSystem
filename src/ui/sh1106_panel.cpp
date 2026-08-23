#include "ui/sh1106_panel.h"

#if BOARD_HAS_LVGL

#include <Arduino.h>
#include <Wire.h>

#include "config.h"  // 链入 platform.h（OLED_I2C_ADDR/分辨率）

namespace {

// ESP32 Arduino Wire 发送缓冲（I2C_BUFFER_LENGTH=128），单次传输上限留余量
constexpr size_t kChunk = 32;

void cmd(uint8_t c) {
  Wire.beginTransmission(OLED_I2C_ADDR);
  Wire.write(0x00);  // control byte: 后续为命令
  Wire.write(c);
  Wire.endTransmission();
}

void writeData(const uint8_t *p, size_t n) {
  while (n > 0) {
    const size_t chunk = n > kChunk ? kChunk : n;
    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x40);  // control byte: 后续为数据
    Wire.write(p, chunk);
    Wire.endTransmission();
    p += chunk;
    n -= chunk;
  }
}

}  // namespace

bool sh1106_panel_begin() {
  Wire.beginTransmission(OLED_I2C_ADDR);
  if (Wire.endTransmission() != 0) {
    return false;  // 屏未在线
  }
  const uint8_t kInit[] = {
      0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0xA1, 0xC8, 0xDA,
      0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0x8D, 0x14, 0xAF};
  for (uint8_t c : kInit) {
    cmd(c);
  }
  return true;
}

void sh1106_panel_draw_1bpp(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                            const uint8_t *px_map) {
  const int32_t w = x2 - x1 + 1;
  // px_map 为 LVGL 1bpp(I1) 位图。LVGL 9 partial 模式下 layer->buf_area==刷新区，
  // 行 0 对应 y1，stride = ceil(区宽/8)（lv_draw_buf_reshape LV_STRIDE_AUTO，
  // lv_draw_buf_width_to_stride = round_up(w*bpp/8, LV_DRAW_BUF_STRIDE_ALIGN=1)）；
  // 全区宽时即 LVGL_HOR_RES/8=16，与计划假设一致。
  const int32_t stride = (w + 7) / 8;
  const int32_t xoff = 2;  // SH1106 常见列偏移 2（部分模块 0，实测可能为 0，需调整）

  for (int32_t page = y1 / 8; page <= y2 / 8; ++page) {
    const int32_t py0 = page * 8;
    uint8_t line[128];
    for (int32_t x = x1; x <= x2; ++x) {
      uint8_t b = 0;
      for (int r = 0; r < 8; ++r) {
        const int32_t yy = py0 + r;
        if (yy < y1 || yy > y2) {
          continue;
        }
        const int32_t byte = (yy - y1) * stride + (x - x1) / 8;
        // LVGL 9 I1 位序 MSB-first：像素 x 位于字节内 7-((x-x1)&7) 位
        // （lv_draw_sw_blend_to_i1.c set_bit/clear_bit）
        const uint8_t bit = static_cast<uint8_t>(
            (px_map[byte] >> (7 - ((x - x1) & 7))) & 1U);
        if (bit) {
          b |= static_cast<uint8_t>(1U << r);
        }
      }
      line[x - x1] = b;
    }
    cmd(static_cast<uint8_t>(0xB0 + (page & 0x07)));
    cmd(static_cast<uint8_t>(0x02 + ((x1 + xoff) & 0x0F)));  // 低列
    cmd(static_cast<uint8_t>(0x10 + ((x1 + xoff) >> 4)));    // 高列
    writeData(line, static_cast<size_t>(w));
  }
}

#endif
