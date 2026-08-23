# 显示屏 v2 适配（SH1106 128×64 I2C）Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development 或 executing-plans 逐任务执行（checkbox 跟踪）。

**Goal:** 把已完成但基于 ST7789 SPI 的显示屏实现，适配为用户选定的 **SH1106 128×64 I2C OLED**（共用 I2C 总线、单色 LVGL、KEY0=返回/KEY1=确认、EC11 编码器）。

**Architecture:** 保留 LVGL 9 与 app_ui 3 屏逻辑；`st7789_panel.*` 替换为 `sh1106_panel.*`（I2C 0x3C，共用 GPIO8/9 Wire）；LVGL 改单色 `LV_COLOR_DEPTH=1`；中文字库 1bpp 重生成；分辨率 128×64；引脚宏收敛。

**Tech Stack:** ESP32-S3 / Arduino / LVGL 9.2 / SH1106(I2C) / Wire / lv_font_conv(1bpp)。

**Spec:** `docs/superpowers/specs/2026-08-22-display-lvgl-design.md`（§8 v2 变更）

**验证方式说明：** 每任务编译验证；硬件点亮待板子到手。工作目录 `/Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem`，分支 master（用户已同意）。`~/.platformio/penv/bin/pio` 为 pio 路径；lv_font_conv 在 `~/.nvm/versions/node/v22.23.2/bin/lv_font_conv`；源字体 `/tmp/fontsrc/NotoSansSC-Regular.otf`（已下载，可复用）。

---

### Task 1: 引脚宏 v2 + lv_conf 单色化

**Files:**
- Modify: `include/boards/board_s3.h`
- Modify: `include/lv_conf.h`

- [ ] **Step 1: board_s3.h 引脚区重写**

用以下内容替换现有 `board_s3.h` 中 LCD/输入组宏（原 §4.1 那组，含 `PIN_LCD_*`、`PIN_ENC_A/B`、`PIN_BTN_OK/BACK`、`LVGL_HOR_RES/VER_RES` 两处定义——先 read 确认现状再替换为）：

```c

// ── OLED 屏（SH1106 128×64，I2C 共用 GPIO8/9，地址 0x3C）──
//   模块 IIC_SCL→PIN_I2C_SCL(9)、IIC_SDA→PIN_I2C_SDA(8)、3V3/GND 供电
#define OLED_I2C_ADDR 0x3C
// ── 编码器/按键组（模块 TRIM_A/TRIM_B/KEY0/KEY1，线序用户自调）──
#define PIN_ENC_A    6    // TRIM_A
#define PIN_ENC_B    7    // TRIM_B
#define PIN_BTN_OK   16   // KEY1=确认
#define PIN_BTN_BACK 17   // KEY0=返回
// ── 屏幕分辨率 ──
#define LVGL_HOR_RES 128
#define LVGL_VER_RES 64
```

> 删除原 `PIN_LCD_SCLK/MOSI/CS/RST/DC/BLK` 6 宏（SPI 屏不再使用）；若 board_io_map.h 中已有 `OLED_I2C_ADDR 0x3C` 定义（之前 grep 见过 `#define OLED_I2C_ADDR 0x3C`），则 board_s3.h 不必重复定义——read 确认后决定（保留其一，避免重定义宏告警；宏重复定义同值在 C 里无告警，但整洁起见只留一处）。

- [ ] **Step 2: lv_conf.h 单色化**

`include/lv_conf.h` 中：

```c
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 1
```

替换为：

```c
#define LV_COLOR_DEPTH 1
```

（删除 `LV_COLOR_16_SWAP`——16bit 专用。若 LVGL 9 对 COLOR_DEPTH=1 需要 `LV_COLOR_1_SWAP` 之类配置，编译报错时按 LVGL 源码提示补充。）

- [ ] **Step 3: 编译**

Run: `cd /Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem && ~/.platformio/penv/bin/pio run -e esp32-s3-irrigation 2>&1 | tail -8`
Expected: 可能报 `st7789_panel`/分辨率相关错（Task 2/3 处理），属计划内；**重点确认 lv_conf 单色配置本身无 #error**（LVGL 头依赖已开）。

- [ ] **Step 4: Commit**

```bash
git add include/boards/board_s3.h include/lv_conf.h
git commit -m "feat(display): v2 引脚（SH1106 I2C）+ LVGL 单色配置"
```

---

### Task 2: sh1106_panel 驱动（替换 st7789_panel）

**Files:**
- Create: `src/ui/sh1106_panel.h`
- Create: `src/ui/sh1106_panel.cpp`
- Delete: `src/ui/st7789_panel.h`、`src/ui/st7789_panel.cpp`

- [ ] **Step 1: 新建 `src/ui/sh1106_panel.h`**

```cpp
#pragma once

#include <stdint.h>

#if BOARD_HAS_LVGL

bool sh1106_panel_begin();
// LVGL 1bpp flush：把脏区 1bpp 位图按 SH1106 页寻址写入
void sh1106_panel_draw_1bpp(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                            const uint8_t *px_map);

#endif
```

- [ ] **Step 2: 新建 `src/ui/sh1106_panel.cpp`**

```cpp
#include "ui/sh1106_panel.h"

#if BOARD_HAS_LVGL

#include <Arduino.h>
#include <Wire.h>

#include "config.h"  // 链入 platform.h（OLED_I2C_ADDR/分辨率）

namespace {

void cmd(uint8_t c) {
  Wire.beginTransmission(OLED_I2C_ADDR);
  Wire.write(0x00);  // control byte: 后续为命令
  Wire.write(c);
  Wire.endTransmission();
}

void writeData(const uint8_t *p, size_t n) {
  Wire.beginTransmission(OLED_I2C_ADDR);
  Wire.write(0x40);  // control byte: 后续为数据
  Wire.write(p, n);
  Wire.endTransmission();
}

}  // namespace

bool sh1106_panel_begin() {
  Wire.beginTransmission(OLED_I2C_ADDR);
  if (Wire.endTransmission() != 0) {
    return false;  // 屏未在线
  }
  const uint8_t kInit[] = {
      0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0xA1, 0xC8, 0xDA,
      0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0x8D, 0x14, 0xAF};
  for (uint8_t c : kInit) {
    cmd(c);
  }
  return true;
}

void sh1106_panel_draw_1bpp(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                            const uint8_t *px_map) {
  const int32_t w = x2 - x1 + 1;
  // px_map 为 LVGL 1bpp 位图，行连续；stride 以 LVGL 实际布局为准
  // （通常每行 ceil(屏宽/8)=16 字节，或按 area 宽；此处按 16 即 LVGL_HOR_RES/8，
  //   若与 LVGL 源码不符，编译后以实测/源码核对修正）
  const int32_t stride = LVGL_HOR_RES / 8;
  const int32_t xoff = 2;  // SH1106 常见列偏移 2（部分模块 0，实测调整）

  for (int32_t page = y1 / 8; page <= y2 / 8; ++page) {
    const int32_t py0 = page * 8;
    uint8_t line[128];
    for (int32_t x = x1; x <= x2; ++x) {
      uint8_t b = 0;
      for (int r = 0; r < 8; ++r) {
        const int32_t yy = py0 + r;
        if (yy < y1 || yy > y2) {
          continue;
        }
        const int32_t byte = (yy - y1) * stride + (x - x1) / 8;
        const uint8_t bit = static_cast<uint8_t>(
            (px_map[byte] >> ((x - x1) & 7)) & 1U);
        if (bit) {
          b |= static_cast<uint8_t>(1U << r);
        }
      }
      line[x - x1] = b;
    }
    cmd(static_cast<uint8_t>(0xB0 + (page & 0x07)));
    cmd(static_cast<uint8_t>(0x02 + ((x1 + xoff) & 0x0F)));  // 低列
    cmd(static_cast<uint8_t>(0x10 + ((x1 + xoff) >> 4)));    // 高列
    writeData(line, static_cast<size_t>(w));
  }
}

#endif
```

> 说明：`stride` 与 `xoff` 是实现关键假设——LVGL 1bpp buffer 的行宽对齐方式与 SH1106 列偏移（0 或 2）需以 LVGL 9 源码与实物为准；实现者若能从 LVGL 源码确认 stride 布局（`lv_display` 1bpp 渲染行对齐），优先按源码写，并在报告中注明。逐像素提取性能可接受（部分刷新、低帧率），若嫌慢可后续优化为行复制。

- [ ] **Step 3: 删除旧文件**

```bash
git rm src/ui/st7789_panel.h src/ui/st7789_panel.cpp
```

- [ ] **Step 4: 编译**

Run: `pio run -e esp32-s3-irrigation 2>&1 | tail -8`
Expected: 报 `lvgl_port.cpp` 引用 st7789（Task 3 处理）属计划内；确认 sh1106_panel 自身编译无误（`OLED_I2C_ADDR`/`LVGL_HOR_RES` 可见——config.h 链 platform.h；若 OLED_I2C_ADDR 未定义，检查 board_io_map.h 已有 `#define OLED_I2C_ADDR 0x3C`）。

- [ ] **Step 5: Commit**

```bash
git add src/ui/sh1106_panel.h src/ui/sh1106_panel.cpp
git commit -m "feat(display): SH1106 I2C 驱动（替换 st7789，1bpp 页寻址 flush）"
```

---

### Task 3: lvgl_port + app_ui 布局适配 128×64

**Files:**
- Modify: `src/ui/lvgl_port.cpp`
- Modify: `src/ui/app_ui.cpp`

- [ ] **Step 1: lvgl_port.cpp 改 flush 与分辨率**

- `#include "ui/st7789_panel.h"` → `#include "ui/sh1106_panel.h"`
- `g_buf`：`lv_color_t g_buf[LVGL_HOR_RES * 20];` → 单色缓冲（1bpp 全屏 1KB）：

```cpp
uint8_t g_buf[LVGL_HOR_RES * LVGL_VER_RES / 8];
```

- `flush_cb`：调用从 `st7789_panel_draw_rgb565(...)` 改为：

```cpp
  sh1106_panel_draw_1bpp(area->x1, area->y1, area->x2, area->y2, px_map);
```

- `lvgl_port_begin`：`lv_display_set_color_format(g_disp, LV_COLOR_FORMAT_RGB565)` 改为单色格式（LVGL 9 单色：`LV_COLOR_FORMAT_L8`? 或移除该调用——1bpp 下 LVGL 默认；按 LVGL 9 API：`lv_display_set_color_format(g_disp, LV_COLOR_FORMAT_I1)`（1-bit indexed）。编译报错时按 LVGL 9 头文件实际枚举修正）
- `lv_display_set_buffers` 的 `sizeof(g_buf)` 自动适配（g_buf 已改 1KB）

- [ ] **Step 2: app_ui.cpp 布局适配 128×64**

坐标按小屏调整（原 320×240 坐标作废）：
- 标题（g_home 内）：`lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 2)`（字号 14，1 行）
- `g_h_status`：`LV_ALIGN_TOP_LEFT, 2, 20`
- 删除 `g_h_daily` 相关（上一版已删）或与状态合并
- `g_h_list`：`lv_obj_set_size(g_h_list, LVGL_HOR_RES - 4, 34); lv_obj_align(..., 2, 34)`（128 宽 64 高：标题 16 + 状态 16 + 列表 34 = 66，微调至不溢出；列表按钮行高自动压缩，14px 字体 2-3 行盆）
- 详情页：`g_d_name` y2、`g_d_info` y22（2 行）、`g_d_vol` y50、按钮 `g_d_btn/g_d_back` 尺寸缩至 `(120, 26)`、对齐 `LV_ALIGN_BOTTOM_MID, 0, -28 / -2`
- 故障页：`ft` y6、`g_f_text` y30、`g_f_btn` 尺寸 `(120,26)` `LV_ALIGN_BOTTOM_MID, 0, -4`
- 盆列表项文本精简（128 宽：`"盆1 45% 正常"` 14px ≈ 80px，够）
- 若高度溢出：主界面列表只显示前 3 盆滚动（`lv_list` 本身可滚，编码器焦点滚动）

> 坐标以"不溢出 + 可读"为目标，实现者按 128×64 实际排版微调（编译无 UI 报错；视觉待硬件）。

- [ ] **Step 3: 编译**

Run: `pio run -e esp32-s3-irrigation 2>&1 | tail -8`
Expected: `SUCCESS`（st7789 引用已清、单色配置生效）。

- [ ] **Step 4: Commit**

```bash
git add src/ui/lvgl_port.cpp src/ui/app_ui.cpp
git commit -m "feat(display): LVGL 单色 flush + 3 屏布局适配 128×64"
```

---

### Task 4: 中文字库 1bpp 重生成

**Files:**
- Modify: `src/ui/lv_font_cn_14.c`

- [ ] **Step 1: 重生成 1bpp 字库**

```bash
cd /tmp/fontsrc
~/.nvm/versions/node/v22.23.2/bin/lv_font_conv --no-compress --font NotoSansSC-Regular.otf \
  -r 0x20-0x7E \
  --symbols "盆浇水开关自动故障恢复返回干转超时限额今日泵阀下限上限水量正在停止清除灌溉中网络连接正常偏干偏湿无效空闲完成检查锁定未知度保护已，" \
  --size 14 --bpp 1 --format lvgl --force-fast-kern-format \
  -o /Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem/src/ui/lv_font_cn_14.c
```

（`--bpp 1` 为单色关键；若 `--no-compress` 与 bpp1 冲突则去掉；确认字体名仍 `lv_font_cn_14`；若 .c 首部 include 需要 `#include "lvgl.h"` 保持。）

- [ ] **Step 2: 编译**

Run: `pio run -e esp32-s3-irrigation 2>&1 | tail -4`
Expected: `SUCCESS`（Flash 应下降，因 1bpp 字库更小）。

- [ ] **Step 3: Commit**

```bash
git add src/ui/lv_font_cn_14.c
git commit -m "feat(display): 中文字库 1bpp 重生成（单色）"
```

---

### Task 5: 编译 + 文档同步

**Files:**
- Modify: `docs/development.md`
- Modify: `docs/DOC_MAP.md`

- [ ] **Step 1: 全量编译**

Run: `cd /Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem && ~/.platformio/penv/bin/pio run -e esp32-s3-irrigation 2>&1 | tail -8`
Expected: `SUCCESS`（RAM ~40%、Flash 略降）。

- [ ] **Step 2: development.md 显示屏节更新**

`docs/development.md` 的"### 显示屏（2026-08-22 接入）"小节更新为：

```markdown
### 显示屏（2026-08-22 接入，v2：SH1106 128×64 I2C）

- SH1106 128×64 OLED，I2C 共用 GPIO8/9（地址 0x3C），LVGL 9 单色（LV_COLOR_DEPTH=1）
- EC11 编码器（TRIM_A/B→GPIO6/7）+ 按键（KEY0=返回→GPIO17、KEY1=确认→GPIO16；PUSH 预留）
- 3 屏：主界面（泵/今日水量 + 盆列表）、盆详情（浇水）、故障页（恢复）
- 中文字库：`src/ui/lv_font_cn_14.c`（Noto Sans SC 14px 1bpp 子集）
- 引脚集中 `include/boards/board_s3.h`（OLED_I2C_ADDR/编码器/按键宏），线序按实际模块调整该处
- 水量=该盆配置 volume_ml；旋钮调水量为后续迭代
```

- [ ] **Step 3: DOC_MAP.md 版本记录追加**

锚点（现有最后一行，显示屏接入 2026-08-22 那条）：

```markdown
| 2026-08-22 | 显示屏接入：ST7789+LVGL 3 屏中文界面（主界面/浇水/故障）、中文字库、引脚宏集中 board_s3.h |
```

在其后追加：

```markdown
| 2026-08-22 | 显示屏 v2：换 SH1106 128×64 I2C（共用总线 0x3C、LVGL 单色 1bpp、EC11+KEY0/KEY1），ST7789 方案作废 |
```

- [ ] **Step 4: Commit**

```bash
git add docs/development.md docs/DOC_MAP.md
git commit -m "docs: 显示屏 v2（SH1106 128×64）适配记录"
```

---

## Self-Review 记录

**Spec 覆盖（§8 v2）：** 8.1 决策 → Task 1/3/4；8.2 引脚 → Task 1；8.3 SH1106 驱动 → Task 2；8.4 单色配置 → Task 1/3/4；8.5 布局 → Task 3。无缺口。

**占位符扫描：** 无 TBD/TODO；sh1106_panel 完整代码、锚点基于已提交代码（board_s3.h v1 宏、lv_conf.h、lvgl_port.cpp、app_ui.cpp）。

**类型一致性：** `sh1106_panel_begin/draw_1bpp`（Task 2 定义 ↔ Task 3 调用）一致；`OLED_I2C_ADDR`（board 头 ↔ 驱动）一致（board_io_map.h 已有 0x3C 定义，注意去重）；`lv_font_cn_14`（Task 4 重生成 ↔ app_ui 引用）一致。

**已知假设（硬件未实测，需到手验证）：** ① SH1106 列偏移 `xoff=2`（部分模块 0）；② LVGL 1bpp buffer stride = HOR_RES/8（以 LVGL 源码为准）；③ 段重映射/COM 扫描方向（0xA1/0xC8）可能需按模块翻转；④ 编码器/按键极性（INPUT_PULLUP + 低有效，input_encoder.cpp 现有逻辑已按此）。

**潜在风险：** LVGL 9 单色模式 API（color format 枚举 I1/L8、1bpp 渲染）与 SH1106 页寻址的配合，编译能过但显示效果待硬件；Task 2 Step 2 已给实现者"按 LVGL 源码核对 stride"的指令。
