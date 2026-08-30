#include "led/status_led.h"

#include <NeoPixelBus.h>

#include "config.h"

// 单颗 WS2812，GRB 顺序，800kHz，RMT 通道 0
// （digitalWrite 无法驱动 WS2812——旧逻辑对板载 RGB 无效）
NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod> g_pixel(1, PIN_STATUS_LED);

namespace {
// 15% 亮度（255×0.15≈38）：满亮度刺眼
constexpr uint8_t kBrightnessScale = 38;
}  // namespace

void statusLedBegin() {
  g_pixel.Begin();
  g_pixel.Show();  // 初始全灭
}

void statusLedSet(uint32_t rgb) {
  const uint8_t r = static_cast<uint8_t>(((rgb >> 16) & 0xFF) * kBrightnessScale / 255);
  const uint8_t g = static_cast<uint8_t>(((rgb >> 8) & 0xFF) * kBrightnessScale / 255);
  const uint8_t b = static_cast<uint8_t>((rgb & 0xFF) * kBrightnessScale / 255);
  g_pixel.SetPixelColor(0, RgbColor(r, g, b));
  g_pixel.Show();
}
