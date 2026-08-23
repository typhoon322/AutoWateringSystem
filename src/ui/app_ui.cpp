#include "ui/app_ui.h"

#if BOARD_HAS_LVGL

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include <lvgl.h>
#include <WiFi.h>

#include "config.h"
#include "control/irrigation_controller.h"
#include "types/zone_config.h"
#include "ui/input_encoder.h"

#include "fonts/lv_font_cn_14.h"

namespace {

SystemContextEx *g_ctx = nullptr;

lv_obj_t *g_scr = nullptr;
lv_obj_t *g_home = nullptr;
lv_obj_t *g_detail = nullptr;
lv_obj_t *g_fault = nullptr;

// home
lv_obj_t *g_h_status = nullptr;
lv_obj_t *g_h_list = nullptr;

// detail
lv_obj_t *g_d_name = nullptr;
lv_obj_t *g_d_info = nullptr;
lv_obj_t *g_d_vol = nullptr;
lv_obj_t *g_d_btn = nullptr;
lv_obj_t *g_d_back = nullptr;

// fault
lv_obj_t *g_f_text = nullptr;
lv_obj_t *g_f_btn = nullptr;

uint8_t g_sel = 0;        // 选中盆
uint16_t g_vol = 100;     // 待浇水量
int8_t g_screen = 0;      // 0=home 1=detail 2=fault

lv_group_t *g_group = nullptr;

const char *stateCn(IrrigationState s) {
  switch (s) {
    case IrrigationState::Idle: return "空闲";
    case IrrigationState::Valving: return "开阀中";
    case IrrigationState::Pumping: return "浇水";
    case IrrigationState::Done: return "完成";
    case IrrigationState::Fault: return "故障";
    default: return "检查";
  }
}

const char *safetyCn(SafetyState s) {
  switch (s) {
    case SafetyState::Ok: return "正常";
    case SafetyState::Timeout: return "超时";
    case SafetyState::DryRun: return "干转";
    case SafetyState::DailyLimit: return "日限额";
    case SafetyState::Locked: return "锁定";
    default: return "未知";
  }
}

const char *pctCn(uint8_t pct, uint8_t low, uint8_t high, bool valid) {
  if (!valid) return "无效";
  if (pct < low) return "偏干";
  if (pct > high) return "偏湿";
  return "正常";
}

void showScreen(int8_t s) {
  g_screen = s;
  lv_obj_add_flag(g_home, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(g_detail, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(g_fault, LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(s == 0 ? g_home : (s == 1 ? g_detail : g_fault), LV_OBJ_FLAG_HIDDEN);
}

void updateHome() {
  if (g_ctx == nullptr) return;
  char line[80];
  snprintf(line, sizeof(line), "泵:%s %s %lu ml",
           g_ctx->status->pump_on ? "开" : "关", stateCn(g_ctx->status->state),
           static_cast<unsigned long>(g_ctx->status->daily_ml));
  lv_label_set_text(g_h_status, line);

  const uint8_t n = g_ctx->config->zone_count > MAX_ZONES ? MAX_ZONES : g_ctx->config->zone_count;
  lv_obj_clean(g_h_list);
  for (uint8_t i = 0; i < n; ++i) {
    const ZoneStatus &zs = g_ctx->zone_status[i];
    const ZoneConfig &zc = g_ctx->zones[i];
    char item[48];
    if (zs.sensor_valid) {
      snprintf(item, sizeof(item), "盆%u  %u%% %s", i + 1, zs.moisture_pct,
               pctCn(zs.moisture_pct, zc.moisture_low, zc.moisture_high, true));
    } else {
      snprintf(item, sizeof(item), "盆%u  -- 无效", i + 1);
    }
    lv_obj_t *btn = lv_list_add_button(g_h_list, LV_SYMBOL_DOWN, item);
    // 字体只设在文本 label 上：直接设按钮会连图标(符号)一起继承 cn 字体，丢失 ↓ 箭头
    uint32_t ci;
    for (ci = 0; ci < lv_obj_get_child_count(btn); ++ci) {
      lv_obj_t *ch = lv_obj_get_child(btn, ci);
      if (lv_obj_check_type(ch, &lv_label_class)) {
        lv_obj_set_style_text_font(ch, &lv_font_cn_14, 0);
        break;
      }
    }
    lv_obj_add_flag(btn, LV_OBJ_FLAG_EVENT_BUBBLE);      // 按键/点击冒泡
    if (g_group != nullptr) {
      lv_group_add_obj(g_group, btn);                    // 旋钮可聚焦
    }
  }
}

void updateDetailInfo() {
  if (g_ctx == nullptr) return;
  const uint8_t i = g_sel;
  const ZoneStatus &zs = g_ctx->zone_status[i];
  const ZoneConfig &zc = g_ctx->zones[i];
  char line[48];
  if (zs.sensor_valid) {
    snprintf(line, sizeof(line), "湿度:%u%% %s\n下限:%u%% 上限:%u%%", zs.moisture_pct,
             pctCn(zs.moisture_pct, zc.moisture_low, zc.moisture_high, true),
             zc.moisture_low, zc.moisture_high);
  } else {
    snprintf(line, sizeof(line), "湿度:无效\n下限:%u%% 上限:%u%%", zc.moisture_low,
             zc.moisture_high);
  }
  lv_label_set_text(g_d_info, line);
}

void updateDetail() {
  if (g_ctx == nullptr) return;
  const uint8_t i = g_sel;
  const ZoneConfig &zc = g_ctx->zones[i];
  char line[48];
  snprintf(line, sizeof(line), "盆%u", i + 1);
  lv_label_set_text(g_d_name, line);
  updateDetailInfo();
  g_vol = zc.volume_ml > 0 ? zc.volume_ml : 100;
  snprintf(line, sizeof(line), "水量:%u ml", g_vol);
  lv_label_set_text(g_d_vol, line);
}

void updateFault() {
  if (g_ctx == nullptr) return;
  char line[64];
  snprintf(line, sizeof(line), "%s保护 泵已停", safetyCn(g_ctx->status->safety));
  lv_label_set_text(g_f_text, line);
}

void updateAll() {
  if (g_ctx == nullptr) return;
  if (g_ctx->status->state == IrrigationState::Fault) {
    if (g_screen != 2) {
      updateFault();
      showScreen(2);
    }
    return;
  }
  if (g_screen == 2) {
    showScreen(0);
    updateHome();
    return;
  }
  if (g_screen == 0) {
    updateHome();
  }
}

void openDetail(uint8_t zone) {
  g_sel = zone;
  updateDetail();
  showScreen(1);
}

void onListClick(lv_event_t *e) {
  if (g_ctx == nullptr) return;
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *parent = lv_obj_get_parent(btn);
  uint32_t idx = 0;
  const uint32_t cnt = lv_obj_get_child_count(parent);
  for (uint32_t k = 0; k < cnt; ++k) {
    if (lv_obj_get_child(parent, k) == btn) {
      idx = k;
      break;
    }
  }
  openDetail(static_cast<uint8_t>(idx));
}

void onDetailWater(lv_event_t *) {
  if (g_ctx == nullptr || g_ctx->controller == nullptr) return;
  g_ctx->controller->requestManual(g_sel, g_vol);
  showScreen(0);
  updateHome();
}

void onDetailBack(lv_event_t *) {
  showScreen(0);
  updateHome();
}

void onFaultRecover(lv_event_t *) {
  if (g_ctx != nullptr && g_ctx->controller != nullptr) {
    g_ctx->controller->stop();
  }
  showScreen(0);
  updateHome();
}

void onScreenKey(lv_event_t *e) {
  const uint32_t key = lv_event_get_key(e);
  if (key == LV_KEY_ESC && g_screen != 0) {
    if (g_screen == 1) {
      showScreen(0);
      updateHome();
    } else if (g_screen == 2 && g_ctx != nullptr &&
               g_ctx->status->state != IrrigationState::Fault) {
      showScreen(0);
      updateHome();
    }
  }
}

void setupGroup() {
  g_group = input_encoder_group();
  if (g_group == nullptr) {
    return;
  }
  lv_obj_add_flag(g_h_list, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_flag(g_d_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_flag(g_d_back, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_flag(g_f_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_group_add_obj(g_group, g_h_list);
  lv_group_add_obj(g_group, g_d_btn);
  lv_group_add_obj(g_group, g_d_back);
  lv_group_add_obj(g_group, g_f_btn);
}

}  // namespace

void app_ui_begin(SystemContextEx *ctx) {
  g_ctx = ctx;

  g_scr = lv_screen_active();
  lv_obj_set_style_bg_color(g_scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_add_event_cb(g_scr, onScreenKey, LV_EVENT_KEY, nullptr);

  g_home = lv_obj_create(g_scr);
  lv_obj_set_size(g_home, LVGL_HOR_RES, LVGL_VER_RES);
  lv_obj_set_style_bg_opa(g_home, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(g_home, 0, 0);

  lv_obj_t *t = lv_label_create(g_home);
  lv_label_set_text(t, "自动灌溉");
  lv_obj_set_style_text_font(t, &lv_font_cn_14, 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 2);

  g_h_status = lv_label_create(g_home);
  lv_obj_set_style_text_font(g_h_status, &lv_font_cn_14, 0);
  lv_obj_align(g_h_status, LV_ALIGN_TOP_LEFT, 2, 20);

  g_h_list = lv_list_create(g_home);
  lv_obj_set_size(g_h_list, LVGL_HOR_RES - 4, 32);
  lv_obj_align(g_h_list, LV_ALIGN_TOP_LEFT, 2, 36);
  lv_obj_add_event_cb(g_h_list, onListClick, LV_EVENT_CLICKED, nullptr);

  g_detail = lv_obj_create(g_scr);
  lv_obj_set_size(g_detail, LVGL_HOR_RES, LVGL_VER_RES);
  lv_obj_set_style_bg_opa(g_detail, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(g_detail, 0, 0);

  g_d_name = lv_label_create(g_detail);
  lv_obj_set_style_text_font(g_d_name, &lv_font_cn_14, 0);
  lv_obj_align(g_d_name, LV_ALIGN_TOP_MID, 0, 2);

  g_d_info = lv_label_create(g_detail);
  lv_obj_set_style_text_font(g_d_info, &lv_font_cn_14, 0);
  lv_obj_align(g_d_info, LV_ALIGN_TOP_LEFT, 2, 22);

  g_d_vol = lv_label_create(g_detail);
  lv_obj_set_style_text_font(g_d_vol, &lv_font_cn_14, 0);
  lv_obj_align(g_d_vol, LV_ALIGN_TOP_LEFT, 2, 48);

  g_d_btn = lv_button_create(g_detail);
  lv_obj_set_size(g_d_btn, 120, 26);
  lv_obj_align(g_d_btn, LV_ALIGN_BOTTOM_MID, 0, -30);
  lv_obj_add_event_cb(g_d_btn, onDetailWater, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *bl = lv_label_create(g_d_btn);
  lv_label_set_text(bl, "浇水");
  lv_obj_set_style_text_font(bl, &lv_font_cn_14, 0);
  lv_obj_center(bl);

  g_d_back = lv_button_create(g_detail);
  lv_obj_set_size(g_d_back, 120, 26);
  lv_obj_align(g_d_back, LV_ALIGN_BOTTOM_MID, 0, -2);
  lv_obj_add_event_cb(g_d_back, onDetailBack, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *bl2 = lv_label_create(g_d_back);
  lv_label_set_text(bl2, "返回");
  lv_obj_set_style_text_font(bl2, &lv_font_cn_14, 0);
  lv_obj_center(bl2);

  g_fault = lv_obj_create(g_scr);
  lv_obj_set_size(g_fault, LVGL_HOR_RES, LVGL_VER_RES);
  lv_obj_set_style_bg_opa(g_fault, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(g_fault, 0, 0);

  lv_obj_t *ft = lv_label_create(g_fault);
  lv_label_set_text(ft, "故障");
  lv_obj_set_style_text_font(ft, &lv_font_cn_14, 0);
  lv_obj_align(ft, LV_ALIGN_TOP_MID, 0, 6);

  g_f_text = lv_label_create(g_fault);
  lv_obj_set_style_text_font(g_f_text, &lv_font_cn_14, 0);
  lv_obj_align(g_f_text, LV_ALIGN_TOP_MID, 0, 28);

  g_f_btn = lv_button_create(g_fault);
  lv_obj_set_size(g_f_btn, 120, 26);
  lv_obj_align(g_f_btn, LV_ALIGN_BOTTOM_MID, 0, -4);
  lv_obj_add_event_cb(g_f_btn, onFaultRecover, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *fl = lv_label_create(g_f_btn);
  lv_label_set_text(fl, "恢复");
  lv_obj_set_style_text_font(fl, &lv_font_cn_14, 0);
  lv_obj_center(fl);

  showScreen(0);
  setupGroup();
  updateHome();
}

void app_ui_update() {
  if (g_ctx == nullptr) return;
  updateAll();
  if (g_screen == 1) {
    updateDetailInfo();  // 仅刷新湿度行，不重置水量/不重建控件
  }
}

#endif
