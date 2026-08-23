#include "ui/lvgl_port.h"

#include "config.h"  // MAX_ZONES/PIN_*/LVGL_HOR_RES/LVGL_VER_RES（链入 platform.h）

#if BOARD_HAS_LVGL

#include <Arduino.h>

#include <lvgl.h>

#include "ui/input_encoder.h"
#include "ui/st7789_panel.h"

namespace {

lv_display_t *g_disp = nullptr;
lv_color_t g_buf[LVGL_HOR_RES * 20];
uint32_t g_last_tick_ms = 0;

void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  const int32_t w = area->x2 - area->x1 + 1;
  const int32_t h = area->y2 - area->y1 + 1;
  st7789_panel_draw_rgb565(area->x1, area->y1, area->x2, area->y2,
                           reinterpret_cast<const uint16_t *>(px_map));
  lv_display_flush_ready(disp);
}

}  // namespace

bool lvgl_port_begin() {
  lv_init();

  if (!st7789_panel_begin()) {
    return false;
  }

  g_disp = lv_display_create(LVGL_HOR_RES, LVGL_VER_RES);
  lv_display_set_flush_cb(g_disp, flush_cb);
  lv_display_set_buffers(g_disp, g_buf, nullptr, sizeof(g_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_color_format(g_disp, LV_COLOR_FORMAT_RGB565);

  input_encoder_begin();
  g_last_tick_ms = millis();
  return true;
}

void lvgl_port_loop() {
  const uint32_t now = millis();
  const uint32_t elapsed = now - g_last_tick_ms;
  if (elapsed > 0) {
    lv_tick_inc(elapsed);
    g_last_tick_ms = now;
  }

  input_encoder_poll();
  lv_timer_handler();
}

#endif
