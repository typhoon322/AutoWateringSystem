#pragma once

#include "config.h"

#if BOARD_HAS_STATUS_OLED

void panel_buttons_begin();
void panel_buttons_poll();

#endif
