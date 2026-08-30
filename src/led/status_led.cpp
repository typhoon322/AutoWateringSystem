#include "led/status_led.h"

#include <NeoPixelBus.h>

#include "config.h"

// 单颗 WS2812，GRB 顺序，800kHz，RMT 通道 0
// （digitalWrite 无法驱动 WS2812——旧逻辑对板载 RGB 无效）
// 用 NeoPixelBrightnessBus 变体以获得 SetBrightness（全局亮度）
NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod> g_pixel(1, PIN_STATUS_LED);

void statusLedBegin() {
  g_pixel.Begin();
  g_pixel.SetBrightness(38);  // 15% 亮度（255×0.15≈38），满亮度刺眼
  g_pixel.Show();             // 初始全灭
}

void statusLedSet(uint32_t rgb) {
  g_pixel.SetPixelColor(
      0, RgbColor(static_cast<uint8_t>((rgb >> 16) & 0xFF),
                  static_cast<uint8_t>((rgb >> 8) & 0xFF),
                  static_cast<uint8_t>(rgb & 0xFF)));
  g_pixel.Show();
}
