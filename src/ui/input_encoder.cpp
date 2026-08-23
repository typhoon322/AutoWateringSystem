#include "ui/input_encoder.h"

#if BOARD_HAS_LVGL

#include <Arduino.h>

#include <lvgl.h>

#include "config.h"  // MAX_ZONES 先于 board_io_map.h 可见

#include "boards/board_s3.h"

namespace {
constexpr int8_t kEncTable[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

lv_indev_t *g_indev = nullptr;
lv_group_t *g_group = nullptr;

volatile int32_t g_enc_diff = 0;
uint8_t g_enc_last = 0;

bool g_btn_ok = false;
bool g_btn_back = false;

void encoder_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
  (void)indev;

  data->enc_diff = g_enc_diff;
  g_enc_diff = 0;

  if (g_btn_ok) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->key = LV_KEY_ENTER;
  } else if (g_btn_back) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->key = LV_KEY_ESC;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
    data->key = 0;
  }
}
}  // namespace

void input_encoder_begin() {
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_BTN_OK, INPUT_PULLUP);
  pinMode(PIN_BTN_BACK, INPUT_PULLUP);

  g_enc_last = (static_cast<uint8_t>(digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B));

  g_group = lv_group_create();
  g_indev = lv_indev_create();
  lv_indev_set_type(g_indev, LV_INDEV_TYPE_ENCODER);
  lv_indev_set_read_cb(g_indev, encoder_read_cb);
  lv_indev_set_group(g_indev, g_group);
}

void input_encoder_poll() {
  const uint8_t curr =
      (static_cast<uint8_t>(digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B));
  const uint8_t idx = static_cast<uint8_t>((g_enc_last << 2) | curr);
  g_enc_diff += kEncTable[idx & 0x0F];
  g_enc_last = curr;

  g_btn_ok = digitalRead(PIN_BTN_OK) == LOW;
  g_btn_back = digitalRead(PIN_BTN_BACK) == LOW;
}

lv_indev_t *input_encoder_indev() { return g_indev; }

lv_group_t *input_encoder_group() { return g_group; }

#endif
