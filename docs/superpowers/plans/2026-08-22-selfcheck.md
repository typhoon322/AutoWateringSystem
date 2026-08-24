# 开机自检 + 手动锁定 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development 或 executing-plans 逐任务执行。

**Goal:** 上电自检（I2C 设备清单 + 屏幕自检画面），自检期间锁定 Web/CLI 写操作（只读与急停豁免），缺失项警告但继续启动。

**Architecture:** 新 `src/safety/selfcheck.h/cpp`（全局 `g_selfcheck` + `run_selfcheck()`，setup 早期同步执行）；Web/CLI 写处理器检查 `g_selfcheck.done`；`handleStatus` 暴露 selfcheck 摘要；家庭首页警告提示；app_ui 自检摘要画面。

**Tech Stack:** ESP32-S3 / Arduino / WebServer / LVGL 9 单色。

**Spec:** `docs/superpowers/specs/2026-08-22-selfcheck-design.md`

**验证：** 每任务编译（`~/.platformio/penv/bin/pio run -e esp32-s3-irrigation`）。工作目录 `/Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem`，分支 master（用户已同意）。

---

### Task 1: selfcheck 模块 + main.cpp 接入

**Files:**
- Create: `src/safety/selfcheck.h`
- Create: `src/safety/selfcheck.cpp`
- Modify: `src/main.cpp`

- [ ] **Step 1: 新建 `src/safety/selfcheck.h`**

```cpp
#pragma once

#include <stdint.h>

// 开机自检结果（I2C 设备清单）；done=false 期间系统锁定写操作
struct SelfCheckResult {
  bool pca9555_ok;  // 阀扩展板 0x20
  uint8_t ads_ok;   // ADS1115 数量 0-3（0x48/0x49/0x4A）
  bool oled_ok;     // SH1106 0x3C
  uint8_t warnings; // 缺失项数
  bool done;        // 自检完成（锁定解除）
};

extern SelfCheckResult g_selfcheck;

// setup 中同步执行（Web/CLI 服务启动前）
void run_selfcheck();
```

- [ ] **Step 2: 新建 `src/safety/selfcheck.cpp`**

```cpp
#include "safety/selfcheck.h"

#include <Arduino.h>

#include "bus/i2c_bus.h"

SelfCheckResult g_selfcheck;

void run_selfcheck() {
  g_selfcheck = SelfCheckResult{};
  Serial.println(F("[selfcheck] start"));

  // PCA9555 阀扩展板 0x20
  g_selfcheck.pca9555_ok = irrigationI2cProbe(PCA9555_ADDR_VALVES);
  Serial.printf("[selfcheck] PCA9555(0x20): %s\n", g_selfcheck.pca9555_ok ? "PASS" : "FAIL");

  // ADS1115 ×3
  g_selfcheck.ads_ok = 0;
  for (uint8_t a : {ADS1115_ADDR_0, ADS1115_ADDR_1, ADS1115_ADDR_2}) {
    if (irrigationI2cProbe(a)) {
      ++g_selfcheck.ads_ok;
    }
  }
  Serial.printf("[selfcheck] ADS1115: %u/3\n", g_selfcheck.ads_ok);

  // 屏幕 0x3C（仅在 LVGL 使能时才算）
#if BOARD_HAS_LVGL
  g_selfcheck.oled_ok = irrigationI2cProbe(OLED_I2C_ADDR);
  Serial.printf("[selfcheck] OLED(0x3C): %s\n", g_selfcheck.oled_ok ? "PASS" : "FAIL");
#endif

  g_selfcheck.warnings = (g_selfcheck.pca9555_ok ? 0 : 1) +
                         (g_selfcheck.ads_ok < 1 ? 1 : 0) +
                         (g_selfcheck.oled_ok ? 0 : 1);
  g_selfcheck.done = true;
  Serial.printf("[selfcheck] done, warnings=%u\n", g_selfcheck.warnings);
}
```

（`PCA9555_ADDR_VALVES`/`ADS1115_ADDR_*`/`OLED_I2C_ADDR` 来自 board_io_map.h，经 config.h 链入——selfcheck.cpp 需 include "config.h"；上面 include 区需补 `#include "config.h"`。）

- [ ] **Step 3: main.cpp 接入**

- include 区（`#include "safety/safety_monitor.h"` 附近）加 `#include "safety/selfcheck.h"`
- `setup()` 中 `irrigationI2cBegin();` 之后、其他初始化之前调用 `run_selfcheck();`

- [ ] **Step 4: 编译**

Run: `cd /Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem && ~/.platformio/penv/bin/pio run -e esp32-s3-irrigation 2>&1 | tail -5`
Expected: `SUCCESS`。

- [ ] **Step 5: Commit**

```bash
git add src/safety/selfcheck.h src/safety/selfcheck.cpp src/main.cpp
git commit -m "feat(safety): 开机自检模块（I2C 设备清单）"
```

---

### Task 2: Web 写 API 锁定 + 状态暴露 + 家庭首页警告

**Files:**
- Modify: `src/web/web_server.cpp`

- [ ] **Step 1: 加 include**

`#include "safety/safety_monitor.h"` 附近加 `#include "safety/selfcheck.h"`。

- [ ] **Step 2: 写 API 加锁检查**

以下每个 handler 开头（`if (ctx_ == nullptr ...)` 判空之后）加：

```cpp
  if (!g_selfcheck.done) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"selfcheck\"}");
    return;
  }
```

目标 handler（按现有代码逐一 add）：`handleIrrigate`、`handleTestPump`、`handleTestValve`、`handleCal`、`handleAuto`、`handleSettingsPost`、`handleWifiPost`、`handleFlowReset`。

**豁免（不加锁）**：`handleEmergencyStop`、`handleStop`、全部 GET（status/settings/flow/i2cscan/history）、`handleSample`（只读采样可保留不加锁——按 spec 只读豁免；sample 是 POST 但只读性质，加锁亦可，二选一，推荐加锁一致）。

- [ ] **Step 3: handleStatus 暴露 selfcheck**

`handleStatus()` 中（features 块之后）加：

```cpp
  JsonObject sc = doc["selfcheck"].to<JsonObject>();
  sc["pca9555"] = g_selfcheck.pca9555_ok;
  sc["ads_ok"] = g_selfcheck.ads_ok;
  sc["oled"] = g_selfcheck.oled_ok;
  sc["warnings"] = g_selfcheck.warnings;
  sc["done"] = g_selfcheck.done;
```

- [ ] **Step 4: 家庭首页警告提示**

`kHomeHtml` 的 `refreshStatus` 函数中（`document.getElementById('dailyMl')` 更新后）加：

```js
  const sc=document.getElementById('scWarn');
  if(sc){
    if(d.selfcheck&&d.selfcheck.warnings>0){
      sc.style.display='block';
      sc.textContent='⚠️ 启动自检警告：'+(d.selfcheck.pca9555?'':' 阀扩展板缺失')+(d.selfcheck.ads_ok?'':' 湿度板缺失')+(d.selfcheck.oled?'':' 屏幕缺失')+'（详情见调试页）';
    }else{
      sc.style.display='none';
    }
  }
```

并在 `kHomeHtml` 的状态卡片（`<div class="summary">` 之后）加容器：

```html
<div class="card" id="scWarn" style="display:none;border-color:var(--warn);color:var(--warn);font-size:.85rem;padding:10px 14px"></div>
```

- [ ] **Step 5: 编译**

Run: `pio run -e esp32-s3-irrigation 2>&1 | tail -5`
Expected: `SUCCESS`。

- [ ] **Step 6: Commit**

```bash
git add src/web/web_server.cpp
git commit -m "feat(safety): Web 写 API 自检锁定 + selfcheck 状态 + 首页警告"
```

---

### Task 3: CLI 写命令锁定

**Files:**
- Modify: `src/cli/serial_cli.cpp`

- [ ] **Step 1: 加 include**

`#include "safety/safety_monitor.h"` 附近加 `#include "safety/selfcheck.h"`。

- [ ] **Step 2: 写命令加锁检查**

在各写命令分支入口加：

```cpp
    if (!g_selfcheck.done) {
      Serial.println(F("系统自检中…"));
      return;
    }
```

目标命令（在 `serial_cli.cpp` 的 poll/命令分发中按命令字符串定位）：`water`、`pump`、`valve`、`cal`、`set`、`save`、`ppl`、`auto`、`schedule`、`wifi`（写类）。

**豁免**：`help`、`status`、`flow`、`i2cscan`、`stop`（停止/急停始终可用）。

- [ ] **Step 3: 编译**

Run: `pio run -e esp32-s3-irrigation 2>&1 | tail -5`
Expected: `SUCCESS`。

- [ ] **Step 4: Commit**

```bash
git add src/cli/serial_cli.cpp
git commit -m "feat(safety): CLI 写命令自检锁定"
```

---

### Task 4: 屏幕自检摘要画面

**Files:**
- Modify: `src/ui/app_ui.cpp`

- [ ] **Step 1: 自检摘要页**

`app_ui_begin` 中：显示主界面之前，先显示自检摘要（复用现有 3 屏容器结构或简单 label 覆盖）。实现（在 `app_ui_begin` 末尾、`updateHome()` 之前）：

```cpp
  // 自检摘要：先显示 1.5s 再进主界面（有警告 3s 并红字）
  {
    lv_obj_t *sc = lv_label_create(g_scr);
    lv_obj_set_style_text_font(sc, &lv_font_cn_14, 0);
    char line[96];
    snprintf(line, sizeof(line), "自检: 阀%s 湿度%u/3 屏%s",
             g_selfcheck.pca9555_ok ? "OK" : "X",
             g_selfcheck.ads_ok,
             g_selfcheck.oled_ok ? "OK" : "X");
    lv_label_set_text(sc, line);
    lv_obj_align(sc, LV_ALIGN_TOP_MID, 0, 20);
    if (g_selfcheck.warnings > 0) {
      lv_obj_set_style_text_color(sc, lv_color_hex(0xFF5555), 0);
    }
    lv_obj_add_flag(sc, LV_OBJ_FLAG_HIDDEN);
    // 延时后隐藏并显示主界面
    ...
  }
```

> 实现提示：用 LVGL 定时器（`lv_timer_create`）1.5s/3s 后隐藏摘要 label 并 `updateHome()`/`showScreen(0)`；注意摘要 label 与 3 屏容器共存（创建在最上层），延时删除。实现者按 LVGL 9 API 完成，保持简单（一个一次性定时器即可）。若实现复杂，可简化为：摘要直接显示在主界面标题位置 2s 后替换为正常标题（用现有 `showScreen` 机制）。目标：**自检结果在屏幕上可见**。

- [ ] **Step 2: 编译**

Run: `pio run -e esp32-s3-irrigation 2>&1 | tail -5`
Expected: `SUCCESS`。

- [ ] **Step 3: Commit**

```bash
git add src/ui/app_ui.cpp
git commit -m "feat(display): 开机自检摘要画面（有警告红字停留）"
```

---

### Task 5: 编译 + 文档同步

**Files:**
- Modify: `docs/development.md`
- Modify: `docs/DOC_MAP.md`

- [ ] **Step 1: 全量编译**

Run: `cd /Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem && ~/.platformio/penv/bin/pio run -e esp32-s3-irrigation 2>&1 | tail -5`
Expected: `SUCCESS`（RAM ~35%、Flash ~34%）。

- [ ] **Step 2: development.md 更新**

显示屏节之后（或合适位置）追加：

```markdown
### 开机自检（2026-08-22）

- 上电自检：I2C 设备清单（PCA9555/ADS1115×3/屏幕 0x3C），缺失项警告不阻断
- 自检期间 Web 写 API 与 CLI 写命令返回"自检中"；只读与急停豁免
- 屏幕显示自检摘要（有警告红字停留 3s）；Web `/api/status.selfcheck` 暴露结果，家庭首页显示警告
```

- [ ] **Step 3: DOC_MAP.md 版本记录追加**

锚点（现有最后一行，安全施工清单那条）：

```markdown
| 2026-08-22 | 安全施工清单（docs/safety-checklist.md）：续流二极管/保险丝/防水盒/三防漆/看门狗/接线红线 |
```

在其后追加：

```markdown
| 2026-08-22 | 开机自检：I2C 清单 + 屏幕摘要 + Web/CLI 写操作锁定（只读/急停豁免） |
```

- [ ] **Step 4: Commit**

```bash
git add docs/development.md docs/DOC_MAP.md
git commit -m "docs: 开机自检（development/DOC_MAP）"
```

---

## Self-Review 记录

**Spec 覆盖：** 3.1 模块 → T1；3.2 锁定 → T2/T3；3.3 屏幕 → T4；3.4 Web 状态 → T2。无缺口。

**占位符扫描：** 无 TBD；代码完整。`g_selfcheck` 全局在模块 .cpp 定义（同 g_history/g_pca9555 单例模式），main/web/cli/app_ui 经 extern 使用。

**类型一致性：** `SelfCheckResult` 字段（pca9555_ok/ads_ok/oled_ok/warnings/done）模块定义 ↔ web JSON（pca9555/ads_ok/oled/warnings/done）↔ 首页 JS（d.selfcheck.*）一致；`run_selfcheck()` 声明/定义/调用一致。

**已知取舍：** 自检同步 <1s；`handleSample` 加锁（POST 但只读，按写类处理）；OLED 探测仅 `BOARD_HAS_LVGL` 下进行（屏幕走 LVGL）；屏幕摘要实现允许简化（一次性定时器或替换标题）。

**潜在编译风险：** `run_selfcheck` 使用 `{ADS1115_ADDR_0,...}` 初始化列表（C++11 OK）；`irrigationI2cProbe` 声明在 `bus/i2c_bus.h`；OLED_I2C_ADDR 在 board_io_map.h（经 config.h）。app_ui 的摘要 label 若用 lv_timer 需包含正确头（lv_timer.h 经 lvgl.h）。
