# WebUI 家庭模式 + 浇水历史 + 时间窗口 — 设计文档

> 日期：2026-08-22 | 状态：待实现 | 项目：AutoIrrigation v2.1 | 目标固件：esp32-s3-irrigation

## 1. 背景与目标

现有 Web dashboard 是"开发/联调向"（测试控制台、参数表单一锅炖），普通用户（家人）无法日常使用。
本设计将 Web UI 分为**家庭首页 + 调试页**双页面，并新增两项能力：

- **浇水历史**：记录每次浇水（时间/盆/水量/触发方式），**NVS 持久化**（重启不清零）
- **浇水时间窗口**：自动（阈值）浇水只在可配置的时间段内触发——避免夏季中午浇水伤植物

## 2. 决策记录（用户确认）

| 决策点 | 结论 |
|---|---|
| 页面形态 | **双页面**：`/` 家庭首页 + `/dev` 调试页 |
| 调试页内容 | 现有 dashboard HTML **原样搬移**，功能零删减，互链 |
| 家庭首页模块 | 湿度卡片、一键浇水、状态摘要条、故障横幅+恢复、自动开关、最近浇水记录 |
| 浇水记录深度 | **完整历史列表**（环形队列 50 条），NVS 持久化 |
| 时间窗口粒度 | **全局窗口 + 每盆覆盖**（默认跟随全局） |
| 窗口外语义 | **只禁自动（阈值）触发**；手动、定时照常 |
| 实现方案 | 方案 A：`kHomeHtml` + `kDevHtml` 两份字符串 + 独立路由 |

## 3. 现状盘点（勿破坏）

### 3.1 现有 Web 结构（`src/web/web_server.cpp`）

- `kDashboardHtml`：单页面（状态卡/测试控制台/系统参数/分区配置/WiFi）——将整体改名为 `kDevHtml`
- 路由：`/` + `/api/*`（status/settings/irrigate/stop/emergency-stop/test/pump/test/valve/cal/flow/flow/reset/i2cscan/sample/auto/wifi）
- `handleRoot()` 发 dashboard——将改发家庭首页

### 3.2 数据结构（`include/types/zone_config.h`）

```c
struct ZoneConfig {
  char name[16]; uint8_t moisture_low, moisture_high; uint16_t volume_ml;
  bool auto_enabled; bool schedule_enabled; uint8_t schedule_hour, schedule_minute;
  uint16_t cal_dry, cal_wet; bool schedule_fired_today;
  // + window 覆盖字段（见 §4.3）
};
struct SystemConfig {
  uint16_t max_run_sec; uint32_t daily_limit_ml; uint16_t pulses_per_liter;
  uint8_t dry_run_sec; uint8_t zone_count; bool wifi_enabled;
  char wifi_ssid[33]; char wifi_pass[65];
  // + 全局窗口字段（见 §4.3）
};
```

### 3.3 存储（`src/storage/settings_store.cpp`）

- Preferences，namespace `irrigation`，`NVS_MAGIC=0x49525247` 校验
- 新增 key 缺省时用默认值，**无需 bump magic**（旧 NVS 兼容）

### 3.4 控制器（`src/control/irrigation_controller.cpp`）

- `checkAutoTriggers()`：阈值触发点（窗口逻辑插入处）；`finishSession()`：历史记录插入处（成功完成、vol>0 时）
- `IrrigateTrigger`：`None/Threshold/Schedule/Manual`

## 4. 设计

### 4.1 浇水历史（新模块 `src/storage/irrigation_history.h/cpp`）

```cpp
struct IrrigationRecord {
  uint32_t ts;        // unix 时间（NTP；未同步为 0）
  uint8_t  zone;      // 0-9
  uint16_t volume_ml; // 实际浇水量
  uint8_t  trigger;   // IrrigateTrigger: 1=阈值 2=定时 3=手动
};
// 环形队列 50 条，NVS blob key "hist"（50×8=400B）
class IrrigationHistory {
 public:
  bool begin();                              // NVS 加载
  void add(const IrrigationRecord& r);       // 追加 + putBytes 持久化
  uint8_t count() const;
  const IrrigationRecord* get(uint8_t i) const; // 0=最新
};
extern IrrigationHistory g_history;
```

- **持久化时机**：每次 `add()` 立即写 NVS（浇水频率低，NVS 寿命无虞）
- **记录触发点**：`IrrigationController::finishSession()` 非 fault 且 `vol>0` 时 `g_history.add()`
- **API**：`GET /api/history` → `{"records":[{ts,zone,volume_ml,trigger:"manual|threshold|schedule"},...]}` 新→旧，最多 50 条
- 依赖 NTP（固件已有 `configTime`）；无网时 ts=0，前端显示 `--`

### 4.2 家庭首页 `/`（`kHomeHtml`，新）

移动优先单列深色卡片，模块：

| 模块 | 内容 | 数据/动作 |
|---|---|---|
| 状态摘要条 | 泵/当前阀/今日总量/队列 | `/api/status` 2s 轮询 |
| 故障横幅 | 原因 + "恢复运行"按钮 | `state=FAULT` 时显示；POST `/api/stop` |
| 湿度卡片×N | 盆名 + 湿度% + 状态色（<low 红偏干 / low-high 绿正常 / >high 蓝偏湿 / 无效灰未接）；点击展开 | `/api/status.zones[]` |
| 展开操作 | 自动开关（POST `/api/auto`）+ 一键浇水（默认 volume_ml 可改，POST `/api/irrigate`，进行中禁用） | — |
| 自动时段 | "自动浇水时段 17:00-21:00"（只读；有盆覆盖显示"盆3 例外 19:00-21:00"） | `/api/settings` |
| 最近浇水 | 倒序 10 条：时间+盆名+触发+水量 | `/api/history` |
| 底部 | 小字"调试页"链接 → `/dev` | — |

- 复用现有深色主题变量；卡片/字号/按钮更大（触屏友好）
- 断线自愈：轮询 try/catch，显示"连接中断·重试中"

### 4.3 浇水时间窗口

**字段（新增）**：

```c
// SystemConfig
bool   auto_window_enabled = true;  // 全局窗口总开关
uint8_t auto_win_sh = 17, auto_win_sm = 0;   // 开始 17:00
uint8_t auto_win_eh = 21, auto_win_em = 0;   // 结束 21:00

// ZoneConfig（每盆覆盖，默认关闭=跟随全局）
bool   window_override = false;
uint8_t win_sh, win_sm, win_eh, win_em;      // 默认 17:00-21:00
```

**触发逻辑**（`checkAutoTriggers` 阈值分支）：

```
若 该盆 auto_enabled && 湿度低：
  窗口 = 盆.window_override ? 盆窗口 : 全局窗口
  若 全局窗口启用 且 当前时间在窗口内 → enqueue(Threshold)
  否则 → 跳过（下次采样再查）
定时 schedule / 手动 → 不受窗口限制
```

- **跨日支持**：`start > end`（如 22:00-02:00）按跨午夜判定
- 窗口外不累积、不补兑（YAGNI）

**WebUI 呈现**：家庭首页只读显示；调试页系统参数区可编辑全局窗口（开关+起止时间）、分区配置每盆可编辑覆盖窗口。

**NVS key（settings_store 新增）**：

```
aWinEn / aWinSH / aWinSM / aWinEH / aWinEM      // 全局
zWinE{i} / zWinSH{i} / zWinSM{i} / zWinEH{i} / zWinEM{i}  // 盆覆盖
```

**API**：窗口参数随 `/api/settings` GET/POST 收发（并入现有 settings JSON）。

### 4.4 调试页 `/dev`（`kDevHtml` = 现有 dashboard 搬移）

- `kDashboardHtml` 改名 `kDevHtml`，标题改"AutoIrrigation · 调试"，顶部加"← 返回主页"
- 全部现有 JS/功能（测试控制台/泵计时/ppl/安全测试/步骤引导/标定/I2C/流量/参数/WiFi）原样保留
- 新路由 `server.on("/dev", ...)` → `handleDev()`；`handleRoot()` 改发 `kHomeHtml`

### 4.5 后端改动清单

| 文件 | 改动 |
|---|---|
| `include/types/zone_config.h` | SystemConfig/ZoneConfig 加窗口字段 |
| `src/storage/irrigation_history.h/cpp` | **新建**（环形队列 + NVS） |
| `src/storage/settings_store.cpp` | 窗口字段读写（load/save/applyDefaults） |
| `src/control/irrigation_controller.cpp` | `checkAutoTriggers` 窗口判定 + `finishSession` 记录历史 |
| `src/main.cpp` | `g_history` 实例化 + `begin()` |
| `src/web/web_server.cpp` | `kHomeHtml` 新页、`kDevHtml` 改名、`/dev` 路由、`/api/history` 端点、settings 增窗口字段 |

## 5. 明确不做（YAGNI）

- 不做跨日补兑逻辑（窗口外不浇、不累积）
- 不做逐次浇水明细到 NVS 之外（环形队列 50 条封顶）
- 不做每盆自动模式以外的调度增强（保留现有 schedule）
- 家庭首页不做实时刷新以外的动画/图表

## 6. 验证计划

1. `pio run -e esp32-s3-irrigation` 编译通过
2. 烧录 → `/` 家庭首页正常（卡片/状态/浇水/记录）；`/dev` 调试页全功能
3. 浇水一次 → `/api/history` 出现记录；**重启后记录仍在**（NVS 验证）
4. 时间窗口：设 17:00-21:00，改系统时间到窗口外 → 自动不触发；窗口内 → 触发；手动/定时不受限；跨日窗口正确
5. 断线自愈、故障横幅恢复

## 7. 风险与缓解

| 风险 | 缓解 |
|---|---|
| NVS 写入频繁磨损 | 浇水频率低（~20 次/天），400B blob 写入寿命充裕 |
| 历史记录时间戳依赖 NTP | 未同步 ts=0，前端显示 `--`，不阻塞 |
| 双 HTML 增加 Flash | 现用 25%，预计 +25KB（~1%），无压力 |
| 窗口判定与既有 schedule 冲突 | schedule 明确不受窗口限制，文档说明 |
