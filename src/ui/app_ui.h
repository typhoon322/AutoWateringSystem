#pragma once

#include <stdint.h>

#if BOARD_HAS_LVGL

struct SystemContextEx;

void app_ui_begin(SystemContextEx *ctx);
void app_ui_update();

#endif
