#pragma once

#include <stdint.h>

#if BOARD_HAS_LVGL

bool sh1106_panel_begin();
// LVGL 1bpp flush：把脏区 1bpp 位图按 SH1106 页寻址写入
void sh1106_panel_draw_1bpp(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                            const uint8_t *px_map);

#endif
