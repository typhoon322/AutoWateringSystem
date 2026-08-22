# WebUI 家庭模式 + 浇水历史 + 时间窗口 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Web UI 拆为家庭首页（`/`）与调试页（`/dev`）；新增 NVS 持久化的浇水历史（50 条环形队列 + `/api/history`）；新增全局+每盆覆盖的浇水时间窗口（窗口外禁自动阈值触发）。

**Architecture:** 后端新增 `IrrigationHistory` 模块（NVS blob 持久化）；`settings_store` 增加窗口字段；`irrigation_controller` 加窗口判定与历史记录钩子；`web_server` 增加 `kHomeHtml` 页面、`/dev` 路由、`/api/history` 端点，现有 dashboard 改名 `kDevHtml` 原样搬移。

**Tech Stack:** ESP32-S3 / Arduino / PlatformIO / 内嵌 HTML+JS / ArduinoJson / NVS(Preferences)。

**Spec:** `docs/superpowers/specs/2026-08-22-webui-home-mode-design.md`

**验证方式说明：** 无单元测试框架（嵌入式）；每任务编译验证（`~/.platformio/penv/bin/pio run -e esp32-s3-irrigation`），最终浏览器手动验证。工作目录 `/Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem`，分支 master（用户已同意直接实现）。

---

### Task 1: 数据结构 + settings_store 窗口字段

**Files:**
- Modify: `include/types/zone_config.h`
- Modify: `src/storage/settings_store.cpp`

- [ ] **Step 1: zone_config.h 增加窗口字段**

锚点（现有）：

```c
struct ZoneConfig {
  char name[16];
  uint8_t moisture_low;
  uint8_t moisture_high;
  uint16_t volume_ml;
  bool auto_enabled;
  bool schedule_enabled;
  uint8_t schedule_hour;
  uint8_t schedule_minute;
  uint16_t cal_dry;
  uint16_t cal_wet;
  bool schedule_fired_today;
};
```

替换为：

```c
struct ZoneConfig {
  char name[16];
  uint8_t moisture_low;
  uint8_t moisture_high;
  uint16_t volume_ml;
  bool auto_enabled;
  bool schedule_enabled;
  uint8_t schedule_hour;
  uint8_t schedule_minute;
  uint16_t cal_dry;
  uint16_t cal_wet;
  bool schedule_fired_today;
  bool window_override;             // 覆盖全局时间窗口
  uint8_t win_sh, win_sm;           // 覆盖窗口开始 HH:MM
  uint8_t win_eh, win_em;           // 覆盖窗口结束 HH:MM
};
```

锚点（现有）：

```c
struct SystemConfig {
  uint16_t max_run_sec;
  uint32_t daily_limit_ml;
  uint16_t pulses_per_liter;
  uint8_t dry_run_sec;
  uint8_t zone_count;
  bool wifi_enabled;
  char wifi_ssid[33];
  char wifi_pass[65];
};
```

替换为：

```c
struct SystemConfig {
  uint16_t max_run_sec;
  uint32_t daily_limit_ml;
  uint16_t pulses_per_liter;
  uint8_t dry_run_sec;
  uint8_t zone_count;
  bool wifi_enabled;
  char wifi_ssid[33];
  char wifi_pass[65];
  bool auto_window_enabled;         // 全局自动浇水时间窗口总开关
  uint8_t auto_win_sh, auto_win_sm; // 窗口开始（默认 17:00）
  uint8_t auto_win_eh, auto_win_em; // 窗口结束（默认 21:00）
};
```

- [ ] **Step 2: settings_store.cpp 默认值**

在 `applyDefaults` 的 `ctx.config->wifi_pass = ...` 之后追加：

```cpp
  ctx.config->auto_window_enabled = true;
  ctx.config->auto_win_sh = 17;
  ctx.config->auto_win_sm = 0;
  ctx.config->auto_win_eh = 21;
  ctx.config->auto_win_em = 0;
```

在 `applyDefaults` 循环内 `ctx.zones[i].schedule_fired_today = false;` 之后追加：

```cpp
    ctx.zones[i].window_override = false;
    ctx.zones[i].win_sh = 17; ctx.zones[i].win_sm = 0;
    ctx.zones[i].win_eh = 21; ctx.zones[i].win_em = 0;
```

- [ ] **Step 3: settings_store.cpp 加载**

在 `load` 的 `ctx.config->wifi_enabled = ...` 之后追加：

```cpp
  ctx.config->auto_window_enabled = prefs.getUChar("aWinEn", 1) != 0;
  ctx.config->auto_win_sh = prefs.getUChar("aWinSH", 17);
  ctx.config->auto_win_sm = prefs.getUChar("aWinSM", 0);
  ctx.config->auto_win_eh = prefs.getUChar("aWinEH", 21);
  ctx.config->auto_win_em = prefs.getUChar("aWinEM", 0);
```

在 `load` 循环内 `ctx.zones[i].schedule_fired_today = false;` 之后追加：

```cpp
    ctx.zones[i].window_override = prefs.getUChar(zoneKey("zWinE", i), 0) != 0;
    ctx.zones[i].win_sh = prefs.getUChar(zoneKey("zWinSH", i), 17);
    ctx.zones[i].win_sm = prefs.getUChar(zoneKey("zWinSM", i), 0);
    ctx.zones[i].win_eh = prefs.getUChar(zoneKey("zWinEH", i), 21);
    ctx.zones[i].win_em = prefs.getUChar(zoneKey("zWinEM", i), 0);
```

> `zoneKey` 的 `keyBuf[8]` 容量检查：前缀最长 `zWinSH`（6 字符）+ 1 位数字 = 7 + '\0' = 8 ✅。

- [ ] **Step 4: settings_store.cpp 保存**

在 `save` 的 `prefs.putUChar("wifiEn", ...)` 之后追加：

```cpp
  prefs.putUChar("aWinEn", ctx.config->auto_window_enabled ? 1 : 0);
  prefs.putUChar("aWinSH", ctx.config->auto_win_sh);
  prefs.putUChar("aWinSM", ctx.config->auto_win_sm);
  prefs.putUChar("aWinEH", ctx.config->auto_win_eh);
  prefs.putUChar("aWinEM", ctx.config->auto_win_em);
```

在 `save` 循环内 `prefs.putUShort(zoneKey("zWet", i), ...)` 之后追加：

```cpp
    prefs.putUChar(zoneKey("zWinE", i), ctx.zones[i].window_override ? 1 : 0);
    prefs.putUChar(zoneKey("zWinSH", i), ctx.zones[i].win_sh);
    prefs.putUChar(zoneKey("zWinSM", i), ctx.zones[i].win_sm);
    prefs.putUChar(zoneKey("zWinEH", i), ctx.zones[i].win_eh);
    prefs.putUChar(zoneKey("zWinEM", i), ctx.zones[i].win_em);
```

- [ ] **Step 5: 编译**

Run: `cd /Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem && ~/.platformio/penv/bin/pio run -e esp32-s3-irrigation 2>&1 | tail -4`
Expected: `SUCCESS`。

- [ ] **Step 6: Commit**

```bash
git add include/types/zone_config.h src/storage/settings_store.cpp
git commit -m "feat(storage): 浇水时间窗口字段（全局+每盆覆盖）与 NVS 持久化"
```

---

### Task 2: 浇水历史模块 + main.cpp 实例化

**Files:**
- Create: `src/storage/irrigation_history.h`
- Create: `src/storage/irrigation_history.cpp`
- Modify: `src/main.cpp`

- [ ] **Step 1: 新建 `src/storage/irrigation_history.h`**

```cpp
#pragma once

#include <stdint.h>

struct IrrigationRecord {
  uint32_t ts;        // unix 时间（NTP；未同步为 0）
  uint8_t  zone;      // 0-9
  uint16_t volume_ml; // 实际浇水量
  uint8_t  trigger;   // IrrigateTrigger: 1=阈值 2=定时 3=手动
};

// 环形队列 50 条，NVS blob 持久化（重启不清零）
class IrrigationHistory {
 public:
  bool begin();                          // 从 NVS 加载
  void add(const IrrigationRecord& r);   // 追加并持久化
  uint8_t count() const { return count_; }
  const IrrigationRecord* get(uint8_t i) const;  // 0=最新

 private:
  static constexpr uint8_t kMax = 50;
  IrrigationRecord ring_[kMax];
  uint8_t count_ = 0;
};

extern IrrigationHistory g_history;
```

- [ ] **Step 2: 新建 `src/storage/irrigation_history.cpp`**

```cpp
#include "storage/irrigation_history.h"

#include <Preferences.h>
#include <string.h>

namespace {
Preferences histPrefs;
constexpr char kNs[] = "irrigation";
constexpr char kKey[] = "hist";
}  // namespace

IrrigationHistory g_history;

bool IrrigationHistory::begin() {
  if (!histPrefs.begin(kNs, false)) {
    return false;
  }
  uint8_t raw[1 + kMax * sizeof(IrrigationRecord)];
  const size_t n = histPrefs.getBytes(kKey, raw, sizeof(raw));
  if (n < 1) {
    return true;  // 无历史
  }
  count_ = raw[0] > kMax ? kMax : raw[0];
  if (count_ > 0) {
    memcpy(ring_, raw + 1, count_ * sizeof(IrrigationRecord));
  }
  return true;
}

void IrrigationHistory::add(const IrrigationRecord& r) {
  if (count_ < kMax) {
    ring_[count_++] = r;
  } else {
    memmove(ring_, ring_ + 1, (kMax - 1) * sizeof(IrrigationRecord));
    ring_[kMax - 1] = r;
  }
  uint8_t raw[1 + kMax * sizeof(IrrigationRecord)];
  raw[0] = count_;
  memcpy(raw + 1, ring_, count_ * sizeof(IrrigationRecord));
  histPrefs.putBytes(kKey, raw, 1 + count_ * sizeof(IrrigationRecord));
}

const IrrigationRecord* IrrigationHistory::get(uint8_t i) const {
  if (i >= count_) {
    return nullptr;
  }
  return &ring_[count_ - 1 - i];  // 0=最新
}
```

- [ ] **Step 3: main.cpp 实例化 + begin()**

锚点（现有，main.cpp 顶部 extern 区）：

```cpp
extern SettingsStore g_settings;
extern PumpDriver g_pump;
```

在其后追加：

```cpp
#include "storage/irrigation_history.h"
```

（若无 include 则加到顶部 include 区 `#include "storage/settings_store.h"` 附近。）

锚点（现有，全局实例区）：

```cpp
SettingsStore g_settings;
```

在其后追加：

```cpp
IrrigationHistory g_history;
```

锚点（现有，setup 内）：

```cpp
  g_settings.begin();
```

替换为：

```cpp
  g_settings.begin();
  g_history.begin();
```

- [ ] **Step 4: 编译**

Run: `pio run -e esp32-s3-irrigation 2>&1 | tail -4`
Expected: `SUCCESS`。

- [ ] **Step 5: Commit**

```bash
git add src/storage/irrigation_history.h src/storage/irrigation_history.cpp src/main.cpp
git commit -m "feat(history): 浇水历史环形队列 + NVS 持久化模块"
```

---

### Task 3: controller — 窗口判定 + 历史记录钩子

**Files:**
- Modify: `src/control/irrigation_controller.h`
- Modify: `src/control/irrigation_controller.cpp`

- [ ] **Step 1: 头文件加私有方法声明**

锚点（现有）：

```cpp
  void processQueue();
  void syncActuatorStatus();
  uint16_t sessionVolumeMl() const;
```

替换为：

```cpp
  void processQueue();
  void syncActuatorStatus();
  uint16_t sessionVolumeMl() const;
  bool inAutoWindow(uint8_t zone) const;
```

- [ ] **Step 2: 实现 `inAutoWindow`（含跨午夜判定）**

在 `irrigation_controller.cpp` 的 `sessionVolumeMl()` 函数之后追加：

```cpp
bool IrrigationController::inAutoWindow(uint8_t zone) const {
  if (!config_->auto_window_enabled) {
    return true;  // 窗口总开关关闭 = 不限时
  }
  uint8_t sh = config_->auto_win_sh, sm = config_->auto_win_sm;
  uint8_t eh = config_->auto_win_eh, em = config_->auto_win_em;
  if (zone < config_->zone_count && zone_configs_[zone].window_override) {
    sh = zone_configs_[zone].win_sh;
    sm = zone_configs_[zone].win_sm;
    eh = zone_configs_[zone].win_eh;
    em = zone_configs_[zone].win_em;
  }
  const int start = sh * 60 + sm;
  const int end = eh * 60 + em;
  if (start == end) {
    return true;  // 起止相同 = 全天
  }
  time_t now;
  time(&now);
  struct tm ti;
  localtime_r(&now, &ti);
  const int cur = ti.tm_hour * 60 + ti.tm_min;
  if (start < end) {
    return cur >= start && cur < end;
  }
  return cur >= start || cur < end;  // 跨午夜（如 22:00-02:00）
}
```

- [ ] **Step 3: 阈值触发加窗口判定**

锚点（现有）：

```cpp
    if (zc.auto_enabled && zones_->needsWater(z)) {
      enqueue(z, zc.volume_ml, IrrigateTrigger::Threshold);
    }
```

替换为：

```cpp
    if (zc.auto_enabled && zones_->needsWater(z) && inAutoWindow(z)) {
      enqueue(z, zc.volume_ml, IrrigateTrigger::Threshold);
    }
```

- [ ] **Step 4: finishSession 记录历史**

锚点（现有，finishSession 开头）：

```cpp
void IrrigationController::finishSession(bool fault) {
  const uint16_t vol = sessionVolumeMl();
  if (vol > 0) {
    safety_->addDailyMl(vol);
  }
```

替换为（在 addDailyMl 后、泵/阀关闭前记录，此时 active_zone_ 仍有效）：

```cpp
void IrrigationController::finishSession(bool fault) {
  const uint16_t vol = sessionVolumeMl();
  if (vol > 0) {
    safety_->addDailyMl(vol);
  }
  if (!fault && vol > 0) {
    IrrigationRecord rec;
    rec.ts = static_cast<uint32_t>(time(nullptr));
    rec.zone = active_zone_ < MAX_ZONES ? active_zone_ : 0;
    rec.volume_ml = vol;
    rec.trigger = static_cast<uint8_t>(status_->trigger);
    g_history.add(rec);
  }
```

并在文件顶部 include 区（`#include "config.h"` 之后）追加：

```cpp
#include "storage/irrigation_history.h"
```

- [ ] **Step 5: 编译**

Run: `pio run -e esp32-s3-irrigation 2>&1 | tail -4`
Expected: `SUCCESS`。

- [ ] **Step 6: Commit**

```bash
git add src/control/irrigation_controller.h src/control/irrigation_controller.cpp
git commit -m "feat(control): 浇水时间窗口判定 + finishSession 记录历史"
```

---

### Task 4: web_server — /dev 路由、/api/history、settings 窗口字段

**Files:**
- Modify: `src/web/web_server.cpp`

- [ ] **Step 1: settings GET 增加窗口字段**

`handleSettingsGet()` 中锚点（现有）：

```cpp
  doc["zone_count"] = ctx_->config->zone_count;
```

在其后追加：

```cpp
  doc["auto_window_enabled"] = ctx_->config->auto_window_enabled;
  doc["auto_win_sh"] = ctx_->config->auto_win_sh;
  doc["auto_win_sm"] = ctx_->config->auto_win_sm;
  doc["auto_win_eh"] = ctx_->config->auto_win_eh;
  doc["auto_win_em"] = ctx_->config->auto_win_em;
```

`handleSettingsGet()` 中 zones 循环锚点（现有）：

```cpp
    z["cal_dry"] = ctx_->zones[i].cal_dry;
    z["cal_wet"] = ctx_->zones[i].cal_wet;
```

在其后追加：

```cpp
    z["window_override"] = ctx_->zones[i].window_override;
    z["win_sh"] = ctx_->zones[i].win_sh;
    z["win_sm"] = ctx_->zones[i].win_sm;
    z["win_eh"] = ctx_->zones[i].win_eh;
    z["win_em"] = ctx_->zones[i].win_em;
```

- [ ] **Step 2: settings POST 接收窗口字段**

`handleSettingsPost()` 中锚点（现有）：

```cpp
  if (doc["zone_count"].is<uint8_t>()) {
    applyZoneCount(doc["zone_count"]);
  }
```

在其后追加：

```cpp
  if (doc["auto_window_enabled"].is<bool>()) {
    ctx_->config->auto_window_enabled = doc["auto_window_enabled"];
  }
  if (doc["auto_win_sh"].is<uint8_t>()) ctx_->config->auto_win_sh = doc["auto_win_sh"];
  if (doc["auto_win_sm"].is<uint8_t>()) ctx_->config->auto_win_sm = doc["auto_win_sm"];
  if (doc["auto_win_eh"].is<uint8_t>()) ctx_->config->auto_win_eh = doc["auto_win_eh"];
  if (doc["auto_win_em"].is<uint8_t>()) ctx_->config->auto_win_em = doc["auto_win_em"];
```

`handleSettingsPost()` 中 zones 循环锚点（现有）：

```cpp
      if (z["cal_wet"].is<uint16_t>()) {
        ctx_->zones[i].cal_wet = z["cal_wet"];
      }
```

在其后追加：

```cpp
      if (z["window_override"].is<bool>()) {
        ctx_->zones[i].window_override = z["window_override"];
      }
      if (z["win_sh"].is<uint8_t>()) ctx_->zones[i].win_sh = z["win_sh"];
      if (z["win_sm"].is<uint8_t>()) ctx_->zones[i].win_sm = z["win_sm"];
      if (z["win_eh"].is<uint8_t>()) ctx_->zones[i].win_eh = z["win_eh"];
      if (z["win_em"].is<uint8_t>()) ctx_->zones[i].win_em = z["win_em"];
```

- [ ] **Step 3: 新增 `/api/history` 端点**

在 `handleWifiPost()` 之后追加：

```cpp
namespace {
const char *triggerText(uint8_t t) {
  switch (t) {
    case 1: return "threshold";
    case 2: return "schedule";
    case 3: return "manual";
    default: return "unknown";
  }
}
}  // namespace

void WebServerUi::handleHistory() {
  JsonDocument doc;
  JsonArray records = doc["records"].to<JsonArray>();
  const uint8_t n = g_history.count();
  const uint8_t limit = n < 50 ? n : 50;
  for (uint8_t i = 0; i < limit; ++i) {
    const IrrigationRecord *r = g_history.get(i);
    if (r == nullptr) {
      break;
    }
    JsonObject o = records.add<JsonObject>();
    o["ts"] = r->ts;
    o["zone"] = r->zone;
    o["volume_ml"] = r->volume_ml;
    o["trigger"] = triggerText(r->trigger);
  }
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}
```

- [ ] **Step 4: 路由注册**

`setupRoutes()` 中锚点（现有）：

```cpp
  server.on("/", HTTP_GET, [this]() { handleRoot(); });
```

替换为：

```cpp
  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/dev", HTTP_GET, [this]() { handleDev(); });
```

`setupRoutes()` 末尾（现有最后一行）：

```cpp
  server.on("/api/wifi", HTTP_POST, [this]() { handleWifiPost(); });
```

在其后追加：

```cpp
  server.on("/api/history", HTTP_GET, [this]() { handleHistory(); });
```

- [ ] **Step 5: 声明 handleDev/handleHistory**

`web_server.h` 增加方法声明（在现有 public 方法区追加，注意与 `handleRoot` 相邻）：

```cpp
  void handleRoot();
  void handleDev();
  ...
  void handleHistory();
```

（若头文件按声明顺序排列，追加到对应位置；先 read 确认。）

- [ ] **Step 6: 编译**

Run: `pio run -e esp32-s3-irrigation 2>&1 | tail -4`
Expected: `SUCCESS`。若 `handleDev` 未定义报链接错误属预期（Task 5 实现 kHomeHtml/kDevHtml 时补齐），可先行注释路由或接受暂时链接失败——**推荐先执行 Task 5 再统一编译**，本任务结束时暂不编译提交（见 Step 7 说明）。

- [ ] **Step 7: Commit（暂不编译时的处理）**

若 Step 6 编译失败源于 `handleDev` 未定义，则**不单独提交**，与 Task 5 合并提交；否则正常提交：

```bash
git add src/web/web_server.cpp src/web/web_server.h
git commit -m "feat(web): /api/history 端点 + settings 窗口字段 + /dev 路由骨架"
```

---

### Task 5: 页面拆分 — kDevHtml 搬移 + kHomeHtml 家庭首页

**Files:**
- Modify: `src/web/web_server.cpp`

- [ ] **Step 1: kDashboardHtml 改名 kDevHtml + 标题 + 返回链接**

将 `const char kDashboardHtml[] PROGMEM = R"rawliteral(...)rawliteral";` 改名：

```cpp
const char kDevHtml[] PROGMEM = R"rawliteral(<!DOCTYPE html>
```

`<title>AutoIrrigation</title>` 改为：

```html
<title>AutoIrrigation · 调试</title>
```

`<div class="wrap">` 之后第一行 `<h1>AutoIrrigation Web</h1>` 改为：

```html
<h1>AutoIrrigation · 调试</h1>
<p class="hint"><a href="/" style="color:var(--accent)">← 返回主页</a> · 联调/开发用，家人用主页即可</p>
```

- [ ] **Step 2: handleRoot 改发家庭页 + handleDev 发调试页**

锚点（现有）：

```cpp
void WebServerUi::handleRoot() { server.send_P(200, "text/html", kDashboardHtml); }
```

替换为：

```cpp
void WebServerUi::handleRoot() { server.send_P(200, "text/html", kHomeHtml); }
void WebServerUi::handleDev() { server.send_P(200, "text/html", kDevHtml); }
```

- [ ] **Step 3: 新增 kHomeHtml（完整家庭首页）**

在 `kDevHtml` 字符串定义**之前**插入 `kHomeHtml`（完整代码见下，含 HTML/CSS/JS；沿用现有深色主题变量，页面标题/卡片/按钮放大）：

```cpp
const char kHomeHtml[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>AutoIrrigation</title>
<style>
:root{--bg:#0f1419;--card:#1a2332;--accent:#00cc88;--warn:#ff5555;--text:#e6edf3;--muted:#8b949e;--blue:#388bfd}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,sans-serif;background:var(--bg);color:var(--text);padding:12px;line-height:1.5}
.wrap{max-width:520px;margin:0 auto}
h1{font-size:1.3rem;color:var(--accent);margin-bottom:4px}
.card{background:var(--card);border-radius:14px;padding:16px;margin-bottom:12px;border:1px solid #30363d}
.summary{display:flex;gap:10px;flex-wrap:wrap;font-size:.9rem;color:var(--muted);margin-top:6px}
.summary b{color:var(--text)}
.zone{border:1px solid #30363d;border-radius:12px;padding:14px;margin-bottom:10px;background:#0d1117}
.zone-head{display:flex;justify-content:space-between;align-items:center}
.zone-name{font-weight:700;font-size:1.05rem}
.zone-pct{font-size:1.6rem;font-weight:800}
.status-dot{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:6px}
.dry{color:#ff7b72}.dry .status-dot{background:#ff7b72}
.ok{color:var(--accent)}.ok .status-dot{background:var(--accent)}
.wet{color:#79c0ff}.wet .status-dot{background:#79c0ff}
.dead{color:var(--muted)}.dead .status-dot{background:var(--muted)}
.zone-actions{display:none;margin-top:12px;border-top:1px solid #30363d;padding-top:10px}
.zone-actions.open{display:block}
.row{display:flex;justify-content:space-between;align-items:center;margin:6px 0;gap:8px}
label{font-size:.85rem;color:var(--muted)}
input,select{padding:8px;border:1px solid #30363d;border-radius:8px;background:#0d1117;color:var(--text);font-size:1rem;width:100%}
button{padding:12px;border:none;border-radius:10px;font-weight:700;cursor:pointer;font-size:1rem}
.btn-go{background:var(--accent);color:#0f1419;width:100%}
.btn-ghost{background:#21262d;color:var(--text);border:1px solid #30363d}
.btn-warn{background:var(--warn);color:#fff;width:100%}
.switch{position:relative;width:52px;height:28px;background:#30363d;border-radius:14px;border:none;cursor:pointer;transition:background .2s}
.switch.on{background:var(--accent)}
.switch::after{content:'';position:absolute;top:3px;left:3px;width:22px;height:22px;background:#fff;border-radius:50%;transition:left .2s}
.switch.on::after{left:27px}
.rec{display:flex;justify-content:space-between;font-size:.85rem;padding:6px 0;border-bottom:1px solid #21262d;color:var(--text)}
.rec:last-child{border-bottom:none}
.rec .t{color:var(--muted)}
.fault{border-color:var(--warn)!important;background:#2a1515}
.fault h3{color:var(--warn)}
.hint{font-size:.8rem;color:var(--muted);margin:8px 0}
.foot{text-align:center;font-size:.8rem;color:var(--muted);padding:8px 0 20px}
.foot a{color:var(--muted)}
</style>
</head>
<body>
<div class="wrap">
<h1>🌱 AutoIrrigation</h1>
<div class="card">
<div style="font-size:1.05rem;font-weight:700">今日已浇 <b id="dailyMl">—</b> ml</div>
<div class="summary">
<span>泵: <b id="pumpSt">OFF</b></span>
<span>正在浇: <b id="zoneSt">无</b></span>
<span>队列: <b id="queueSt">0</b></span>
<span id="winSt"></span>
</div>
</div>

<div class="card fault" id="faultCard" style="display:none">
<h3>⚠️ 系统故障</h3>
<p class="hint" id="faultText">—</p>
<button class="btn-warn" onclick="doRecover()">恢复运行</button>
</div>

<div class="card" id="zonesCard"><div id="zonesList">加载中…</div></div>

<div class="card">
<div style="font-weight:700;margin-bottom:8px">最近浇水</div>
<div id="recList">加载中…</div>
</div>

<div class="foot"><a href="/dev">调试页</a></div>
</div>
<script>
const MAXZ=10;
let settings={};
async function api(p,o){const r=await fetch(p,o);return r.json()}
function esc(s){return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/"/g,'&quot;')}
function pctColor(z){
  if(!z.sensor_valid)return 'dead';
  if(z.moisture_pct < z.moisture_low)return 'dry';
  if(z.moisture_pct > z.moisture_high)return 'wet';
  return 'ok';
}
function pctLabel(c){return c==='dry'?'偏干':c==='wet'?'偏湿':c==='ok'?'正常':'未接'}
function fmtWin(){
  const s=settings;
  if(!s||s.auto_window_enabled==null)return '';
  if(!s.auto_window_enabled)return '自动不限时';
  const p=(h,m)=>String(h).padStart(2,'0')+':'+String(m).padStart(2,'0');
  return '自动时段 '+p(s.auto_win_sh,s.auto_win_sm)+' - '+p(s.auto_win_eh,s.auto_win_em);
}
function renderZones(zones){
  const el=document.getElementById('zonesList');
  let h='';
  for(let i=0;i<zones.length;i++){
    const z=zones[i]||{};
    const c=pctColor(z);
    const pct=z.moisture_pct!=null?z.moisture_pct:'--';
    h+='<div class="zone"><div class="zone-head" onclick="toggleZone('+i+')">'+
      '<div><span class="zone-name">🪴 '+esc(z.name||('Zone'+i))+'</span><br><span class="'+c+'"><span class="status-dot"></span>'+pctLabel(c)+'</span></div>'+
      '<div class="zone-pct '+c+'">'+pct+'%</div></div>'+
      '<div class="zone-actions" id="za'+i+'">'+
      '<div class="row"><span>自动浇水</span><button class="switch'+(z.auto_enabled?' on':'')+'" onclick="toggleAuto('+i+')"></button></div>'+
      '<div class="row"><label>水量 ml</label><input type="number" id="vol'+i+'" value="'+(z.volume_ml||100)+'" min="10"></div>'+
      '<button class="btn-go" id="btnW'+i+'" onclick="doWater('+i+')">浇水</button></div></div>';
  }
  el.innerHTML=h||'（未配置盆数）';
}
function toggleZone(i){
  const a=document.getElementById('za'+i);
  if(a)a.classList.toggle('open');
}
async function toggleAuto(i){
  const on=!settings.zones[i].auto_enabled;
  await api('/api/auto',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({zone:i,enabled:on})});
  loadAll();
}
async function doWater(i){
  const b=document.getElementById('btnW'+i);
  const vol=+document.getElementById('vol'+i).value;
  if(b)b.disabled=true;
  await api('/api/irrigate',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({zone:i,volume_ml:vol})});
  if(b){b.disabled=false;b.textContent='浇水'}
  setTimeout(refreshStatus,300);
}
async function doRecover(){await api('/api/stop',{method:'POST'});refreshStatus()}
async function refreshStatus(){
  let d=null;
  try{d=await api('/api/status')}catch(e){}
  const live=document.getElementById('dailyMl');
  if(!d){live.textContent='连接中断';return}
  document.getElementById('dailyMl').textContent=d.daily_ml;
  document.getElementById('pumpSt').textContent=d.pump?'ON':'OFF';
  document.getElementById('zoneSt').textContent=(d.valve_on&&d.active_valve>=0)?'盆'+(d.active_valve+1):'无';
  document.getElementById('queueSt').textContent=d.queue;
  const fault=d.fault||d.safety_locked||d.state==='FAULT';
  document.getElementById('faultCard').style.display=fault?'block':'none';
  if(fault)document.getElementById('faultText').textContent='原因: '+(d.safety||'?');
  const zones=(d.zones||[]);
  for(let i=0;i<zones.length;i++){
    const btn=document.getElementById('btnW'+i);
    if(btn){btn.disabled=d.pump;btn.textContent=d.pump?'浇水中…':'浇水'}
  }
}
async function renderHistory(){
  let h=null;
  try{h=await api('/api/history')}catch(e){}
  const el=document.getElementById('recList');
  if(!h||!h.records||!h.records.length){el.textContent='暂无记录';return}
  const names=settings.zones||[];
  const trig={manual:'手动',threshold:'自动',schedule:'定时'};
  el.innerHTML=h.records.slice(0,10).map(r=>{
    const n=names[r.zone]?names[r.zone].name:('盆'+(r.zone+1));
    const t=r.ts?new Date(r.ts*1000).toLocaleString('zh-CN',{hour12:false}):'--';
    return '<div class="rec"><span>'+(trig[r.trigger]||r.trigger)+' · '+esc(n)+' · '+r.volume_ml+'ml</span><span class="t">'+t+'</span></div>';
  }).join('');
}
async function loadAll(){
  try{settings=await api('/api/settings')}catch(e){}
  document.getElementById('winSt').textContent=fmtWin();
  let st=null;
  try{st=await api('/api/status')}catch(e){}
  if(st&&st.zones)renderZones(st.zones);
  renderHistory();
}
loadAll();refreshStatus();setInterval(refreshStatus,2000);
setInterval(function(){renderHistory();},15000);
</script>
</body>
</html>)rawliteral";
```

- [ ] **Step 4: 编译**

Run: `pio run -e esp32-s3-irrigation 2>&1 | tail -6`
Expected: `SUCCESS`（此时 `handleDev`/`handleHistory`/`kHomeHtml`/`kDevHtml` 全部齐备）。若报 `web_server.h` 缺声明，补 `handleDev`/`handleHistory` 声明（Task 4 Step 5）。

- [ ] **Step 5: Commit**

```bash
git add src/web/web_server.cpp src/web/web_server.h
git commit -m "feat(web): 家庭首页 kHomeHtml + 调试页 kDevHtml 双页面拆分"
```

---

### Task 6: 编译 + 验证 + 文档同步

**Files:**
- Modify: `docs/development.md`
- Modify: `docs/user-manual.md`
- Modify: `docs/DOC_MAP.md`

- [ ] **Step 1: 全量编译**

Run: `cd /Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem && ~/.platformio/penv/bin/pio run -e esp32-s3-irrigation 2>&1 | tail -8`
Expected: `SUCCESS`，Flash 增量 ~2-3%。

- [ ] **Step 2: 烧录 + 手动验证清单**

Run: `~/.platformio/penv/bin/pio run -e esp32-s3-irrigation -t upload`

| # | 检查项 | 预期 |
|---|---|---|
| 1 | 打开 `http://<IP>/` | 家庭首页：湿度卡片/状态条/自动时段/最近浇水 |
| 2 | 点击盆卡 | 展开自动开关 + 水量 + 浇水按钮 |
| 3 | 浇水一次 | 状态条更新；最近浇水出现记录 |
| 4 | **重启设备** | 浇水记录仍在（NVS 持久化） |
| 5 | 打开 `/dev` | 原调试页全功能（测试控制台/标定/I2C/流量/ppl/步骤） |
| 6 | `/dev` 顶部"← 返回主页" | 回到家庭首页 |
| 7 | 时间窗口 | 调试页系统参数改全局窗口（如设 08:00-09:00），用串口 `time`/NTP 校准后验证窗口外自动不触发、手动仍可浇 |
| 8 | 故障横幅 | 触发干转（无水浇水）→ 首页显示故障 + "恢复运行"按钮可解除 |
| 9 | 断线自愈 | 断电 10s → "连接中断"；恢复后自动刷新 |

- [ ] **Step 3: development.md 更新**

`docs/development.md` 第 5 节"Web API"开头追加（沿用 2026-08-18 增强段落之后）：

```markdown
### 双页面结构（2026-08-22）

- `/` 家庭首页：湿度卡片、一键浇水、自动模式开关、故障恢复、最近浇水记录、自动时段展示（家人用）
- `/dev` 调试页：测试控制台/参数/标定/I2C/流量/步骤引导（开发联调用）
- `GET /api/history`：最近 50 条浇水记录（NVS 持久化，重启保留）`{records:[{ts,zone,volume_ml,trigger}]}`，trigger: manual|threshold|schedule
- 浇水时间窗口：`auto_window_enabled` + `auto_win_sh/sm/eh/em`（全局），每盆 `window_override` + `win_sh/sm/eh/em`（覆盖）；窗口外自动（阈值）不触发，手动/定时不受限；支持跨午夜
```

- [ ] **Step 4: user-manual.md 更新**

`docs/user-manual.md` 相应章节补"日常使用（家庭首页）"：打开首页即可看湿度/浇水/恢复故障；自动时段说明（默认 17:00-21:00，避免夏季中午浇水）；调试页仅供开发者。具体位置按 user-manual 现有结构追加（先 read 定位）。

- [ ] **Step 5: DOC_MAP.md 版本记录追加**

锚点（现有最后一行，工程布线定稿那条）：

```markdown
| 2026-08-18 | 工程布线方案 v1.0 定稿（docs/engineering-wiring.md）：汇流排/端子排/湿度计线束/10 盆扩展 |
```

在其后追加：

```markdown
| 2026-08-22 | WebUI 家庭模式：`/` 家庭首页 + `/dev` 调试页、浇水历史（NVS 持久化）、浇水时间窗口（全局+每盆覆盖） |
```

- [ ] **Step 6: Commit**

```bash
git add docs/development.md docs/user-manual.md docs/DOC_MAP.md
git commit -m "docs: WebUI 家庭模式/历史记录/时间窗口（development/user-manual/DOC_MAP）"
```

---

## Self-Review 记录

**Spec 覆盖：** 4.1 历史模块 → Task 2；4.3 窗口字段/判定 → Task 1+3；4.2 家庭首页 → Task 5；4.4 调试页 → Task 5；4.5 后端清单 → Task 1-5；`/api/history` → Task 4。无缺口。

**占位符扫描：** 无 TBD/TODO；所有代码完整可粘贴；锚点均来自已读源码（zone_config.h、settings_store.cpp、irrigation_controller.h/cpp、web_server.cpp 全文、main.cpp 关键段）。

**类型一致性：** 窗口字段名（`auto_window_enabled`/`auto_win_sh/sm/eh/em`、`window_override`/`win_sh/sm/eh/em`）在 zone_config.h、settings_store（`aWinEn/aWinSH...`、`zWinE/zWinSH...`）、web_server JSON（同名）全链一致；`IrrigationRecord` 字段（ts/zone/volume_ml/trigger）在模块与 JSON 一致；trigger 数字 1/2/3 ↔ threshold/schedule/manual 在 controller 与 web 映射一致。

**已知取舍：** 历史记录依赖 NTP（ts=0 时前端显示 `--`）；窗口相同起止=全天；不做跨日补兑；家庭首页自动时段只读（改配置在调试页）。

**潜在编译顺序：** Task 4 结束时代码引用 `handleDev`/`kHomeHtml`/`kDevHtml`（Task 5 才定义）——Task 4 与 Task 5 须在同一提交批次内编译验证（计划已注明）。
