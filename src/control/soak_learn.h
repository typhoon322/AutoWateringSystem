#pragma once

#include <stdint.h>

#include "config.h"
#include "types/zone_config.h"

// 浇完后在 MCU 上观察湿度何时稳住，并改每盆的渗水等待。手机断开也继续。
void soak_learn_begin();
void soak_learn_tick(const ZoneStatus *zones, uint8_t count, ZoneConfig *configs);
void soak_learn_on_dose_started(uint8_t zone);
void soak_learn_on_dose_finished(uint8_t zone);
void soak_learn_restart(uint8_t zone);
uint8_t soak_learn_count(uint8_t zone);
bool soak_learn_paused(uint8_t zone);
bool soak_learn_watching(uint8_t zone);
