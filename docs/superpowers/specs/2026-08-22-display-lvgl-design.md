# 显示屏接入（ST7789 + LVGL）— 设计文档

> 日期：2026-08-22 | 状态：待实现 | 项目：AutoIrrigation v2.1 | 目标：esp32-s3-irrigation

## 1. 背景与目标

`src/ui/`（LVGL ST7789 屏 UI）为未完成 WIP：缺引脚/分辨率宏定义、未接入 main.cpp、中文界面需字库。本设计将其收尾为**可编译、逻辑完整、引脚可配置**状态，待板子到手实测引脚后仅改 `board_s3.h` 宏即可点亮。

## 2. 决策记录（用户确认）

| 决策点 | 结论 |
|---|---|
| 交互形态 | **旋钮（编码器）+ 按钮（OK/BACK）交互菜单** |
| 内容范围 | 主界面（状态+盆湿度）、盆详情+手动浇水、故障页+恢复 |
| 明确不做 | 自动开关、完整设置菜单、触摸、浇水历史（Web 已有） |
| 屏幕语言 | **中文字库子集**（Noto Sans SC 常用字 ~14px） |
| 引脚 | 功能分组、冲突全避开；**线序用户自调**，仅改 board_s3.h 宏 |
| 屏幕规格 | ST7789 320×240，SPI（FSPI），LVGL v9 |

## 3. 现状盘点

- `src/ui/`：`lvgl_port.*`（lv_display_create/flush_cb）、`st7789_panel.*`（SPI 驱动+背光）、`input_encoder.*`（旋钮+按钮→LVGL encoder indev）、`app_ui.*`（业务界面）——骨架齐全，缺宏与接线
- `include/lv_conf.h`：已修 C++ 污染问题（不含 board 头）；仅 Montserrat 14/16，需加中文字库
- `platformio.ini` S3 env：`-DBOARD_HAS_LVGL=0`（联调期关闭）→ 本设计恢复 1
- 现有引脚占用：I2C 8/9、泵 4、流量计 5、LED 48、USB 19/20、UART 43/44、strap 0/3/45/46

## 4. 设计

### 4.1 引脚（board_s3.h 追加，分组注释，冲突全避开）

```c
// ── LCD 组（ST7789 SPI，6 线成束）──
#define PIN_LCD_SCLK 12   // J1-18
#define PIN_LCD_MOSI 11   // J1-17
#define PIN_LCD_CS   10   // J1-16
#define PIN_LCD_RST  13   // J1-19
#define PIN_LCD_DC   14   // J1-20
#define PIN_LCD_BLK  15   // J1-8（背光 PWM）
// ── 输入组（编码器+按钮，4 线成束）──
#define PIN_ENC_A    6    // J1-6
#define PIN_ENC_B    7    // J1-7
#define PIN_BTN_OK   16   // J1-9
#define PIN_BTN_BACK 17   // J1-10
// ── 分辨率 ──
#define LVGL_HOR_RES 320
#define LVGL_VER_RES 240
```

> 线序用户自调：拿到实际屏幕/板子后仅改这些宏。J1 物理分布：执行组 4,5 → 输入组 6,7 → LCD-BLK 15 → 输入组 16,17 → I2C 8/9 → LCD 组 10-14（连续）。

### 4.2 main.cpp 接入（`BOARD_HAS_LVGL` 下）

- setup：`lvgl_port_begin()`（失败 WARN 不阻塞）→ `app_ui_begin(&g_ctx_ex)`
- loop：`lvgl_port_loop()`（lv_tick_inc + input poll + lv_timer_handler）+ `app_ui_update()`

### 4.3 界面（3 屏）

```
主界面：顶部状态条（泵/今日水量）+ 盆列表（▸高亮 + 湿度% + 状态色点）
盆详情：盆名 + 湿度 + 上下限 + 水量[±10ml 旋钮] + [浇水]（OK 确认）
故障页（state=FAULT 强制）：原因文字 + [OK 恢复运行]（POST /api/stop 同源）
```

- 旋钮：主界面滚盆；详情页调水量
- OK：主界面进详情；详情确认浇水；故障页恢复
- BACK：返回主界面
- `app_ui_update()` 每 250ms：刷新湿度/状态/故障检测（数据源 g_ctx_ex）
- 盆名显示"盆N"（避免任意中文盆名缺字）

### 4.4 中文字库

- Noto Sans SC 常用字子集（界面词 ~400 字：盆/浇/水/开/关/自动/故障/恢复/返回/干转/超时/限额/今日/泵/阀/下限/上限/水量/正在…），14px
- 工具：`lv_font_conv`（pip 或 lvgl 自带）生成 `.c` 嵌入，约 150-300KB Flash（+~5%）
- `lv_conf.h`：声明该字体为默认（或界面显式指定）

### 4.5 平台配置

- `platformio.ini` S3 env：`-DBOARD_HAS_LVGL=1`（恢复）+ 保留 `-DLV_CONF_INCLUDE_SIMPLE -Iinclude`

## 5. 明确不做（YAGNI）

- 触摸、完整设置菜单、屏幕自动开关、屏幕历史记录、盆覆盖窗口编辑（均在 Web）

## 6. 验证计划

1. 编译 SUCCESS（Flash 预期 ~31%）
2. 硬件（板子到手后）：屏幕点亮→主界面→旋钮滚动→进详情→浇水→故障页+恢复；引脚不符时改 board_s3.h 宏

## 7. 风险与缓解

| 风险 | 缓解 |
|---|---|
| 引脚默认值可能与实际模块不符 | 全部集中宏化，到手实测改一处 |
| 中文字库 Flash 开销 | 子集 ~400 字、14px，+~5% 可接受 |
| LVGL 内存（320×240 部分缓冲） | 现用 PARTIAL 渲染 + g_buf 内部 RAM，RAM 余量充足（14.6%→预计 <30%） |
| 与 Web 并行渲染互不干扰 | app_ui 只读共享数据，无锁冲突（单线程 loop） |

---

## 8. 硬件变更（v2：SH1106 128×64 I2C，2026-08-22 实测模块信息）

> 用户选定模块：1.3 寸 OLED 128×64，SH1106 驱动，**I2C 接口**，板载 EC11 编码器（TRIM_A/B + PUSH）+ 两按键。
> 引脚排针：`+3V3 GND KEY0 TRIM_B TRIM_A PUSH IIC_SCL IIC_SDA KEY1`。KEY0=返回键、KEY1=确认键。
> **原 §4 的 ST7789 320×240 SPI 方案作废**，以下为 v2 适配。

### 8.1 决策

| 项 | 结论 |
|---|---|
| UI 框架 | **保留 LVGL**（复用 app_ui 3 屏逻辑），适配 SH1106 单色模式 |
| 屏幕 I2C | **共用 GPIO8/9**（SH1106 地址 0x3C，与扩展板 0x20/0x48/0x49/0x4A 无冲突），少线零额外代码 |
| 按键 | KEY0 → 返回（PIN_BTN_BACK）、KEY1 → 确认（PIN_BTN_OK）；**PUSH 预留不接** |
| 编码器 | TRIM_A/B → PIN_ENC_A/B |
| 颜色 | LVGL 单色 `LV_COLOR_DEPTH=1`；中文字库 **1bpp 重生成** |
| 分辨率 | 128×64（LVGL_HOR_RES/VER_RES 改） |

### 8.2 引脚（board_s3.h，v2）

```c
// ── OLED 屏（SH1106 128×64，I2C 共用 GPIO8/9，地址 0x3C）──
//   模块 IIC_SCL→PIN_I2C_SCL(9)、IIC_SDA→PIN_I2C_SDA(8)，3V3/GND 供电
#define OLED_I2C_ADDR 0x3C
// ── 编码器/按键组（模块 TRIM_A/TRIM_B/KEY0/KEY1，线序用户自调）──
#define PIN_ENC_A    6
#define PIN_ENC_B    7
#define PIN_BTN_OK   16   // KEY1=确认
#define PIN_BTN_BACK 17   // KEY0=返回
// ── 分辨率 ──
#define LVGL_HOR_RES 128
#define LVGL_VER_RES 64
```

> 删除原 §4.1 的 LCD SPI 组宏（10-15 不再使用）；`BOARD_HAS_OLED`（U8g2 路径）仍为 0，本屏走 LVGL。

### 8.3 SH1106 驱动要点

- 新 `src/ui/sh1106_panel.h/cpp` 替换 `st7789_panel.*`（旧文件删除）
- I2C 复用现有 Wire（GPIO8/9，`irrigationI2cBegin` 已初始化）；地址 0x3C
- LVGL flush 回调：接收 1bpp 脏区缓冲，按 SH1106 **页寻址**重排发送（每 8 行 = 1 page；列偏移 2 为 SH1106 特性，SET_COLUMN 用 0x02/0x10 或按模块实测）
- 初始化序列：SH1106 标准（0xAE→显示关、0xD5/D3 时钟分频、0x40 起始行、0x8D/0x14 电荷泵、0xA8/0x3F 复用、0x81 对比度、0xA1/A0 段重映射按需、0xC8/C0 扫描方向按需、0xAF 显示开）

### 8.4 单色配置（lv_conf.h / lvgl_port）

- `LV_COLOR_DEPTH 1`、删除 `LV_COLOR_16_SWAP`
- `g_buf` = `LVGL_HOR_RES * LVGL_VER_RES / 8` 字节（128×64/8 = 1KB）
- 中文字库 `lv_font_cn_14` **以 `--bpp 1` 重生成**（单色无抗锯齿）

### 8.5 界面布局（128×64）

- 主界面：标题"自动灌溉"（顶部 1 行）+ 状态行（泵/今日，1 行）+ 盆列表 3 行（14px 中文）
- 详情页：盆名 + 湿度/上下限（2-3 行）+ 水量 1 行 + 浇水/返回按钮（底部 2 行）
- 故障页：原因 2 行 + 恢复按钮
- 交互不变（旋钮选盆、KEY1 确认、KEY0 返回、故障强制）
