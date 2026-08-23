#pragma once

#include <stdint.h>

#if BOARD_HAS_LVGL

bool lvgl_port_begin();
void lvgl_port_loop();

#endif
