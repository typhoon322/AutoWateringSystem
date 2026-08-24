# 开机自检 + 手动操作锁定 — 设计文档

> 日期：2026-08-22 | 状态：待实现 | 项目：AutoIrrigation v2.1 | 目标：esp32-s3-irrigation

## 1. 背景与目标

上电开机执行自检，自检期间禁止任何手动操作（Web/CLI/屏幕），防"硬件未就绪就误操作"。自检项聚焦设备清单（I2C 在线性）与屏幕可视反馈；**不做电压检测**（用户判定用处不大，省分压硬件）；**不做继电器机械自检**（无反馈且有真实动作风险）。

## 2. 决策记录（用户确认）

| 决策点 | 结论 |
|---|---|
| 自检项 | I2C 设备清单（PCA9555/ADS1115×3/屏幕）+ 屏幕自检画面 |
| 明确不做 | 电压检测、继电器/泵机械自检、流量计自检、FAIL 阻止启动 |
| 失败策略 | **继续启动 + 警告**（缺失项记录，屏幕红字提示 + Web 状态显示） |
| 锁定范围 | 自检期间 Web 写 API + CLI 写命令拒绝；只读与急停不锁 |
| 锁定机制 | 全局 `g_selfcheck.done` 标志 + 各处理器检查 |

## 3. 设计

### 3.1 新模块 `src/safety/selfcheck.h/cpp`

```cpp
#pragma once
#include <stdint.h>

struct SelfCheckResult {
  bool pca9555_ok;   // 阀扩展板 0x20
  uint8_t ads_ok;    // ADS1115 数量 0-3（0x48/0x49/0x4A）
  bool oled_ok;      // SH1106 0x3C
  uint8_t warnings;  // 缺失项数
  bool done;         // 自检完成（锁定解除）
};

extern SelfCheckResult g_selfcheck;
void run_selfcheck();  // setup 中同步执行
```

`run_selfcheck()`：探测 5 地址（复用 `irrigationI2cProbe`），填充各字段 + `warnings` 计数 + `done=true`。`done` 初始 false（锁定生效直到自检跑完）。

### 3.2 锁定接入点

| 层 | 锁定 | 豁免 |
|---|---|---|
| Web 写 API | `handleIrrigate/handleTestPump/handleTestValve/handleCal/handleAuto/handleSettingsPost/handleWifiPost/handleFlowReset` 开头检查 `!g_selfcheck.done` → 400 `{"ok":false,"error":"selfcheck"}` | `handleEmergencyStop`、`handleStop`、全部 GET（status/settings/flow/i2cscan/history） |
| CLI 写命令 | `water/pump/valve/cal/set/save/ppl/auto/schedule` 分支检查 → `Serial.println("系统自检中…")` | `help/status/flow/i2cscan`、`stop` |
| 屏幕 | 自检画面（app_ui 自检完成前不显示主界面） | — |

> 自检在 `setup()` 中同步执行（Web/CLI 服务启动前），标志作防御层；`done` 由 `run_selfcheck()` 置 true 后服务正常。

### 3.3 屏幕自检画面（app_ui）

- `app_ui_begin` 后若 `!g_selfcheck.done` 显示"自检中…"；完成后逐项显示 PASS/FAIL（复用 `lv_font_cn_14`）
- 有 `warnings>0`：FAIL 项红字（如"PCA9555 缺失"）停留 3s → 主界面；全 PASS 直接主界面
- 实现：app_ui 内部状态（自检画面阶段）或简化——自检在 setup 已完成，app_ui_begin 直接读 `g_selfcheck` 结果绘制摘要页，延时后切主界面

### 3.4 Web 状态暴露

- `handleStatus` 增加 `selfcheck` 对象：`{pca9555, ads_ok, oled, warnings, done}`
- 家庭首页 `kHomeHtml`：`refreshStatus` 读 `selfcheck.warnings>0` 时状态条下显示"⚠️ 启动自检警告：…"（可点击 `/dev` 看详情）

## 4. 明确不做（YAGNI）

- 电压检测、继电器/泵/流量计自检、FAIL 阻止启动、自检日志持久化

## 5. 验证计划

1. 编译 SUCCESS（两个 env）
2. 无硬件：启动串口打印自检摘要（缺失项 WARN），Web 写操作自检期间被拒（可用 `delay` 模拟或直接看 done 前的窗口）
3. 有硬件：正常启动全 PASS；拔掉某 ADS1115 → 警告 + 屏幕红字 + 功能仍可用

## 6. 改动范围

| 文件 | 改动 |
|---|---|
| `src/safety/selfcheck.h/cpp` | 新建 |
| `src/main.cpp` | include + `run_selfcheck()`（setup 早期调用） |
| `src/web/web_server.cpp` | 写 API 加锁检查 + `handleStatus` 加 selfcheck 对象 + `kHomeHtml` 警告提示 |
| `src/cli/serial_cli.cpp` | 写命令加锁检查 |
| `src/ui/app_ui.cpp` | 自检摘要画面 |

## 7. 风险

- 自检同步执行约 <1s（I2C 探测快），不阻塞启动
- 屏幕自检画面若 SH1106 未就绪（oled_ok=false），app_ui 显示 WARN 但系统照常
- 锁定逻辑新增，需确保急停/只读不受影响（代码审查重点）
