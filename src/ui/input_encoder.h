#pragma once

#if BOARD_HAS_LVGL

struct _lv_indev_t;
typedef struct _lv_indev_t lv_indev_t;
struct _lv_group_t;
typedef struct _lv_group_t lv_group_t;

void input_encoder_begin();
void input_encoder_poll();
lv_indev_t *input_encoder_indev();
lv_group_t *input_encoder_group();

#endif
