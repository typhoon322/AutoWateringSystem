#pragma once

#include <stdint.h>

// 板载 WS2812 RGB 状态灯（GPIO48，RMT 驱动，WiFi 常开下不受干扰）
void statusLedBegin();
// rgb: 0xRRGGBB
void statusLedSet(uint32_t rgb);
