#pragma once

#include <stdint.h>

#if BOARD_HAS_LVGL

bool st7789_panel_begin();
void st7789_panel_set_backlight(uint8_t pct);
void st7789_panel_draw_rgb565(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                                const uint16_t *pixels);

#endif
