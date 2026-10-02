#pragma once

#include <stdint.h>

#include "config.h"

#if BOARD_HAS_STATUS_OLED

struct SystemStatus;
struct ZoneStatus;
struct ZoneConfig;

bool status_oled_begin();
void status_oled_set_note(const char *text);
void status_oled_show(const SystemStatus &status, const ZoneStatus *zones,
                      const ZoneConfig *configs, uint8_t zone_count, const char *state_text);

#endif
