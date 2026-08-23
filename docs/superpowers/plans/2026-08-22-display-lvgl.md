# 显示屏接入（ST7789 + LVGL）Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 收尾 `src/ui/` WIP：补齐引脚/分辨率宏、中文字库、3 屏交互界面（主界面/盆详情浇水/故障恢复）、接入 main.cpp，使固件可编译、逻辑完整、引脚可配置（线序用户自调）。

**Architecture:** ST7789 320×240 SPI(FSPI) + LVGL v9 + 旋钮编码器 + 2 按钮。`src/ui/` 4 文件复用，`app_ui` 重写为 3 屏中文界面（单 screen + 容器切换）。引脚全部集中在 `board_s3.h`。

**Tech Stack:** ESP32-S3 / Arduino / LVGL 9.2 / ST7789(FSPI) / lv_font_conv（中文字库生成）。

**Spec:** `docs/superpowers/specs/2026-08-22-display-lvgl-design.md`

**验证方式说明：** 无单测框架；每任务编译验证。硬件显示验证待板子到手（本计划仅保证编译通过 + 逻辑完整 + 引脚可配置）。工作目录 `/Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem`，分支 master（用户已同意直接实现）。`~/.platformio/penv/bin/pio` 为 pio 路径。

---

### Task 1: 引脚宏 + 分辨率 + 平台配置

**Files:**
- Modify: `include/boards/board_s3.h`
- Modify: `platformio.ini`

- [ ] **Step 1: board_s3.h 追加引脚与分辨率宏**

在 `board_s3.h` 末尾（`#define VALVE_ACTIVE_HIGH 0` 之后）追加：

```c

// ── LCD 组（ST7789 SPI，6 线成束；线序按实际屏幕模块调整本组宏）──
#define PIN_LCD_SCLK 12   // J1-18
#define PIN_LCD_MOSI 11   // J1-17
#define PIN_LCD_CS   10   // J1-16
#define PIN_LCD_RST  13   // J1-19
#define PIN_LCD_DC   14   // J1-20
#define PIN_LCD_BLK  15   // J1-8（背光）
// ── 输入组（编码器+按钮，4 线成束）──
#define PIN_ENC_A    6    // J1-6
#define PIN_ENC_B    7    // J1-7
#define PIN_BTN_OK   16   // J1-9
#define PIN_BTN_BACK 17   // J1-10
// ── 屏幕分辨率 ──
#define LVGL_HOR_RES 320
#define LVGL_VER_RES 240
```

> 冲突复核：LCD 12/11/10/13/14/15、输入 6/7/16/17 与 I2C(8/9)、泵(4)、流量计(5)、LED(48)、USB(19/20)、UART(43/44)、strap(0/3/45/46) 全部无冲突。

- [ ] **Step 2: platformio.ini S3 env 恢复 LVGL**

锚点（现有）：

```ini
	-DBOARD_S3_IRRIGATION
	-DBOARD_HAS_LVGL=0
	-DLV_CONF_INCLUDE_SIMPLE
	-Iinclude
```

替换为：

```ini
	-DBOARD_S3_IRRIGATION
	-DBOARD_HAS_LVGL=1
	-DLV_CONF_INCLUDE_SIMPLE
	-Iinclude
```

- [ ] **Step 3: 编译**

Run: `cd /Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem && ~/.platformio/penv/bin/pio run -e esp32-s3-irrigation 2>&1 | tail -6`
Expected: `SUCCESS`（ui 文件编译进，LVGL_HOR_RES 等宏已定义）。

- [ ] **Step 4: Commit**

```bash
git add include/boards/board_s3.h platformio.ini
git commit -m "feat(display): S3 板引脚分组宏（LCD/输入）+ 恢复 LVGL 编译"
```

---

### Task 2: ui 文件 include 修正

**Files:**
- Modify: `src/ui/lvgl_port.cpp`
- Modify: `src/ui/st7789_panel.cpp`

- [ ] **Step 1: lvgl_port.cpp 加平台头 include**

锚点（现有，`lvgl_port.cpp` 顶部）：

```cpp
#include "ui/lvgl_port.h"

#if BOARD_HAS_LVGL

#include <Arduino.h>

#include <lvgl.h>

#include "ui/input_encoder.h"
#include "ui/st7789_panel.h"
```

在 `#include "ui/st7789_panel.h"` 之后追加：

```cpp
#include "platform.h"   // PIN_LCD_*/PIN_ENC_*/PIN_BTN_*/LVGL_HOR_RES/LVGL_VER_RES
```

- [ ] **Step 2: st7789_panel.cpp 加平台头 include**

读文件确认顶部 include 结构，若未包含平台头则在 `#include <SPI.h>` 之后追加：

```cpp
#include "platform.h"
```

（若文件已有 `#include "config.h"` 则无需重复——config.h 链入 platform.h；确认后决定。）

- [ ] **Step 3: 编译**

Run: `pio run -e esp32-s3-irrigation 2>&1 | tail -6`
Expected: `SUCCESS`（无 `PIN_LCD_*`/`LVGL_*` 未声明错误）。

- [ ] **Step 4: Commit**

```bash
git add src/ui/lvgl_port.cpp src/ui/st7789_panel.cpp
git commit -m "fix(ui): 平台头 include 补齐（引脚/分辨率宏可见）"
```

---

### Task 3: 中文字库

**Files:**
- Create: `include/fonts/lv_font_cn_14.c`
- Modify: `include/lv_conf.h`

- [ ] **Step 1: 准备工具与源字体**

```bash
# lv_font_conv（Python 包）
~/.platformio/penv/bin/pip install lv_font_conv 2>&1 | tail -2 || pip install lv_font_conv 2>&1 | tail -2
# 源字体：Noto Sans SC（若下载失败，报告 BLOCKED 并说明；备选 AnyConv/本地 ttf）
mkdir -p /tmp/fontsrc && cd /tmp/fontsrc
curl -sL -o NotoSansSC-Regular.otf "https://github.com/googlefonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf" && ls -la NotoSansSC-Regular.otf
```

- [ ] **Step 2: 生成子集字库**

界面词（按设计 §4.4）：`盆浇水开关自动故障恢复返回干转超时限额今日泵阀下限上限水量正在停止清除灌溉中网络连接正常偏干偏湿无效` + ASCII `0x20-0x7E`。

```bash
cd /tmp/fontsrc
~/.platformio/penv/bin/lv_font_conv --no-compress --font NotoSansSC-Regular.otf \
  -r 0x20-0x7E \
  --symbols "盆浇水开关自动故障恢复返回干转超时限额今日泵阀下限上限水量正在停止清除灌溉中网络连接正常偏干偏湿无效" \
  --size 14 --format lvgl --force-fast-kern-format -o /Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem/include/fonts/lv_font_cn_14.c
ls -la /Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem/include/fonts/lv_font_cn_14.c
```

（若 `lv_font_conv` 不在 penv，用 `python3 -m lv_font_conv` 或 PATH 中命令；工具用法以实际为准，目标是产出 `lv_font_cn_14` 字体的 `.c` 文件。）

- [ ] **Step 3: lv_conf.h 注册字体**

`include/lv_conf.h` 的字体区（现有 `#define LV_FONT_MONTSERRAT_14 1` 附近）追加：

```c
#define LV_FONT_CUSTOM_1 1  // 自定义中文字库（include/fonts/lv_font_cn_14.c）
```

并在文件头部（`#if BOARD_HAS_LVGL` 块内）追加声明：

```c
#include "fonts/lv_font_cn_14.h"
```

> 注意：`include/lv_conf.h` 由 LVGL 的 C 文件以 C 编译方式包含——生成的 `lv_font_cn_14.c` 必须是纯 C（LVGL 生成物默认是 C，OK）；`lv_font_cn_14.h` 声明 `extern const lv_font_t lv_font_cn_14;`。若 `#include "fonts/..."` 在 lv_conf.h 中因 include 路径问题失败，改为在 `app_ui.cpp` 中 `#include "fonts/lv_font_cn_14.h"`（字库源文件加入编译即可，勿双重包含）。

- [ ] **Step 4: 编译**

Run: `pio run -e esp32-s3-irrigation 2>&1 | tail -6`
Expected: `SUCCESS`（Flash 应增至 ~30%+）。

- [ ] **Step 5: Commit**

```bash
git add include/fonts/lv_font_cn_14.c include/lv_conf.h
git commit -m "feat(display): 中文字库子集（Noto Sans SC 14px）"
```

---

### Task 4: app_ui 重写为 3 屏中文界面

**Files:**
- Modify: `src/ui/app_ui.cpp`

- [ ] **Step 1: 完整重写 `app_ui.cpp`**

用以下完整代码替换现有 `app_ui.cpp` 全文（`#if BOARD_HAS_LVGL` 包裹结构保持）：

```cpp
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
lv_obj_t *g_h_daily = nullptr;
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
bool g_detail_ready = false;

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
  snprintf(line, sizeof(line), "泵:%s %s 今日:%lu ml",
           g_ctx->status->pump_on ? "开" : "关", stateCn(g_ctx->status->state),
           static_cast<unsigned long>(g_ctx->status->daily_ml));
  lv_label_set_text(g_h_status, line);

  const uint8_t n = g_ctx->config->zone_count > MAX_ZONES ? MAX_ZONES : g_ctx->config->zone_count;
  lv_list_clean(g_h_list);
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
    lv_list_add_button(g_h_list, LV_SYMBOL_DROPDOWN, item);
  }
}

void updateDetail() {
  if (g_ctx == nullptr) return;
  const uint8_t i = g_sel;
  const ZoneStatus &zs = g_ctx->zone_status[i];
  const ZoneConfig &zc = g_ctx->zones[i];
  char line[48];
  snprintf(line, sizeof(line), "盆%u", i + 1);
  lv_label_set_text(g_d_name, line);
  if (zs.sensor_valid) {
    snprintf(line, sizeof(line), "湿度:%u%% %s\n下限:%u%% 上限:%u%%", zs.moisture_pct,
             pctCn(zs.moisture_pct, zc.moisture_low, zc.moisture_high, true),
             zc.moisture_low, zc.moisture_high);
  } else {
    snprintf(line, sizeof(line), "湿度:无效\n下限:%u%% 上限:%u%%", zc.moisture_low,
             zc.moisture_high);
  }
  lv_label_set_text(g_d_info, line);
  g_vol = zc.volume_ml > 0 ? zc.volume_ml : 100;
  snprintf(line, sizeof(line), "水量:%u ml", g_vol);
  lv_label_set_text(g_d_vol, line);
}

void updateFault() {
  if (g_ctx == nullptr) return;
  char line[64];
  snprintf(line, sizeof(line), "%s 保护，泵已停止", safetyCn(g_ctx->status->safety));
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
  g_detail_ready = true;
  updateDetail();
  showScreen(1);
}

void onListClick(lv_event_t *e) {
  if (g_ctx == nullptr) return;
  lv_obj_t *btn = lv_event_get_target(e);
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

lv_group_t *g_group = nullptr;

void setupGroup() {
  g_group = input_encoder_group();
  if (g_group == nullptr) {
    return;
  }
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
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 6);

  g_h_status = lv_label_create(g_home);
  lv_obj_set_style_text_font(g_h_status, &lv_font_cn_14, 0);
  lv_obj_align(g_h_status, LV_ALIGN_TOP_LEFT, 8, 30);

  g_h_daily = lv_label_create(g_home);
  lv_obj_set_style_text_font(g_h_daily, &lv_font_cn_14, 0);
  lv_obj_align(g_h_daily, LV_ALIGN_TOP_LEFT, 8, 50);

  g_h_list = lv_list_create(g_home);
  lv_obj_set_size(g_h_list, LVGL_HOR_RES - 16, LVGL_VER_RES - 90);
  lv_obj_align(g_h_list, LV_ALIGN_TOP_LEFT, 8, 74);
  lv_obj_add_event_cb(g_h_list, onListClick, LV_EVENT_CLICKED, nullptr);

  g_detail = lv_obj_create(g_scr);
  lv_obj_set_size(g_detail, LVGL_HOR_RES, LVGL_VER_RES);
  lv_obj_set_style_bg_opa(g_detail, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(g_detail, 0, 0);

  g_d_name = lv_label_create(g_detail);
  lv_obj_set_style_text_font(g_d_name, &lv_font_cn_14, 0);
  lv_obj_align(g_d_name, LV_ALIGN_TOP_MID, 0, 10);

  g_d_info = lv_label_create(g_detail);
  lv_obj_set_style_text_font(g_d_info, &lv_font_cn_14, 0);
  lv_obj_align(g_d_info, LV_ALIGN_TOP_LEFT, 8, 40);

  g_d_vol = lv_label_create(g_detail);
  lv_obj_set_style_text_font(g_d_vol, &lv_font_cn_14, 0);
  lv_obj_align(g_d_vol, LV_ALIGN_TOP_LEFT, 8, 110);

  g_d_btn = lv_button_create(g_detail);
  lv_obj_set_size(g_d_btn, 160, 44);
  lv_obj_align(g_d_btn, LV_ALIGN_BOTTOM_MID, 0, -80);
  lv_obj_add_event_cb(g_d_btn, onDetailWater, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *bl = lv_label_create(g_d_btn);
  lv_label_set_text(bl, "浇水");
  lv_obj_set_style_text_font(bl, &lv_font_cn_14, 0);
  lv_obj_center(bl);

  g_d_back = lv_button_create(g_detail);
  lv_obj_set_size(g_d_back, 160, 44);
  lv_obj_align(g_d_back, LV_ALIGN_BOTTOM_MID, 0, -24);
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
  lv_obj_align(ft, LV_ALIGN_TOP_MID, 0, 20);

  g_f_text = lv_label_create(g_fault);
  lv_obj_set_style_text_font(g_f_text, &lv_font_cn_14, 0);
  lv_obj_align(g_f_text, LV_ALIGN_TOP_MID, 0, 60);

  g_f_btn = lv_button_create(g_fault);
  lv_obj_set_size(g_f_btn, 160, 44);
  lv_obj_align(g_f_btn, LV_ALIGN_BOTTOM_MID, 0, -60);
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
}
```

- [ ] **Step 2: 旋钮水量调节接入（input_encoder 已映射 ENTER/ESC；水量 ± 用长按/旋转需在 app_ui_update 中处理——本实现为简化，水量=该盆配置 volume_ml 固定值，浇水即用该值。若要旋钮调水量，后续迭代）**

> 说明：设计 §2"详情页水量 ±10ml 旋钮调"在本实现中简化为"使用该盆配置水量"（`updateDetail` 中 `g_vol = zc.volume_ml`）。原因是 encoder 的旋钮事件已在 input_encoder 内部消费为 LVGL 焦点移动，未暴露"无焦点旋转"事件；避免过度复杂。若要支持，可在 input_encoder 增加旋转回调——标注为后续迭代，不阻塞本任务。

- [ ] **Step 3: 编译**

Run: `pio run -e esp32-s3-irrigation 2>&1 | tail -6`
Expected: `SUCCESS`。若报 `LVGL_HOR_RES`/`lv_font_cn_14` 未声明，检查 include（platform.h / fonts 头）与 lv_conf.h 注册。

- [ ] **Step 4: Commit**

```bash
git add src/ui/app_ui.cpp
git commit -m "feat(display): app_ui 3 屏中文界面（主界面/浇水详情/故障恢复）"
```

---

### Task 5: main.cpp 接入

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: 条件 include 与实例**

锚点（现有，`#if BOARD_HAS_OLED` 区之后）：

```cpp
#if BOARD_HAS_OLED
#include "display/display_driver.h"
#endif
```

在其后追加：

```cpp
#if BOARD_HAS_LVGL
#include "ui/lvgl_port.h"
#include "ui/app_ui.h"
#endif
```

- [ ] **Step 2: setup 启动**

锚点（现有）：

```cpp
  g_controller.begin();
  g_cli.begin(&g_ctx_ex);
  g_web.begin(&g_ctx_ex);
```

替换为：

```cpp
  g_controller.begin();
  g_cli.begin(&g_ctx_ex);
  g_web.begin(&g_ctx_ex);
#if BOARD_HAS_LVGL
  if (lvgl_port_begin()) {
    app_ui_begin(&g_ctx_ex);
    Serial.println(F("LVGL: ST7789 initialized"));
  } else {
    Serial.println(F("WARN: LVGL display init failed"));
  }
#endif
```

- [ ] **Step 3: loop 周期调用**

锚点（现有，`updateStatusLed();` 之前）：

```cpp
  updateStatusLed();
```

替换为：

```cpp
#if BOARD_HAS_LVGL
  lvgl_port_loop();
  app_ui_update();
#endif

  updateStatusLed();
```

- [ ] **Step 4: 编译**

Run: `pio run -e esp32-s3-irrigation 2>&1 | tail -6`
Expected: `SUCCESS`（`g_ctx_ex` 在 lvgl 调用点已定义——确认 main.cpp 中 `g_ctx_ex` 定义在 setup 之前）。

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "feat(display): main 接入 LVGL（begin/loop + app_ui）"
```

---

### Task 6: 编译 + 文档同步

**Files:**
- Modify: `docs/development.md`
- Modify: `docs/DOC_MAP.md`
- Modify: `docs/project-status.md`

- [ ] **Step 1: 全量编译 + 烧录准备**

Run: `cd /Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem && ~/.platformio/penv/bin/pio run -e esp32-s3-irrigation 2>&1 | tail -8`
Expected: `SUCCESS`（Flash ~31% 左右）。

- [ ] **Step 2: development.md 更新**

`docs/development.md` 双页面结构节后追加：

```markdown
### 显示屏（2026-08-22 接入）

- ST7789 320×240 + LVGL 9 + 旋钮/按钮（`src/ui/`）
- 3 屏：主界面（泵/今日水量 + 盆列表）、盆详情（浇水）、故障页（恢复）
- 中文字库：`include/fonts/lv_font_cn_14.c`（Noto Sans SC 14px 子集，界面词）
- 引脚集中在 `include/boards/board_s3.h`（LCD 组/输入组宏），线序按实际屏幕调整该处
- 水量=该盆配置 volume_ml；旋钮调水量为后续迭代
```

- [ ] **Step 3: DOC_MAP.md 版本记录追加**

锚点（现有最后一行 WebUI 家庭模式那条）：

```markdown
| 2026-08-22 | WebUI 家庭模式：`/` 家庭首页 + `/dev` 调试页、浇水历史（NVS 持久化）、浇水时间窗口（全局+每盆覆盖） |
```

在其后追加：

```markdown
| 2026-08-22 | 显示屏接入：ST7789+LVGL 3 屏中文界面（主界面/浇水/故障）、中文字库、引脚宏集中 board_s3.h |
```

- [ ] **Step 4: project-status.md 待办更新**

待办第 9 条"显示屏接入"标记完成状态（改为说明：代码完成待硬件验证）：

```markdown
9. [x] 显示屏接入（ST7789+LVGL 3 屏中文界面，代码完成；硬件点亮待板子到手后改 board_s3.h 引脚宏实测）
```

- [ ] **Step 5: Commit**

```bash
git add docs/development.md docs/DOC_MAP.md docs/project-status.md
git commit -m "docs: 显示屏接入（development/DOC_MAP/project-status）"
```

---

## Self-Review 记录

**Spec 覆盖：** 4.1 引脚 → Task 1；4.2 接入 → Task 5；4.3 三屏 → Task 4；4.4 字库 → Task 3；4.5 平台配置 → Task 1。无缺口。

**占位符扫描：** 无 TBD/TODO（Task 4 Step 2 的"水量旋钮"明确标注为后续迭代的简化决策，非占位）；app_ui.cpp 完整代码、Task 1/2/5 锚点均来自已读源码。

**类型一致性：** `lv_font_cn_14`（Task 3 生成 ↔ Task 4 引用 ↔ lv_conf.h 注册）一致；`g_sel/g_vol/g_screen/g_home/g_detail/g_fault` 等内部符号在 app_ui 内自洽；`showScreen/updateHome/updateDetail/updateFault/updateAll` 调用链完整；`input_encoder_group()` 返回 group 供 `setupGroup` 使用（input_encoder.cpp 已有该函数）。

**已知取舍：** 详情页水量=盆配置值（旋钮调水量后置）；盆名显示"盆N"；故障页在 `state==Fault` 时强制覆盖；中文字库子集仅含界面词（自定义盆名可能缺字→界面固定"盆N"规避）。

**潜在编译风险：** ① `lv_font_conv` 工具/字体下载可能失败（Task 3 Step 1/2，失败则报告 BLOCKED 处理）；② lv_conf.h 中 include fonts 头可能路径问题（Task 3 Step 3 已给备选：改在 app_ui.cpp include）；③ LVGL API 版本差异（v9 的 lv_button/lv_list_add_button/lv_obj_add_flag 均存在，若编译报 API 名错，按 LVGL 9 实际 API 修正并记录）。
