# Web UI 联调测试增强 — 设计文档

> 日期：2026-08-18 | 状态：待实现 | 项目：AutoIrrigation v2.1 | 固件目标：esp32-s3-irrigation

## 1. 背景与目标

硬件进入接线联调阶段（当前先测泵，后续阀/流量计）。现有 Web dashboard 已具备基础测试控制台与状态展示，但存在缺口：

- **状态展示不全**：传感器有效性、编译特性、I2C 设备列表、流量计读数、安全参数当前值未在页面上展示（接口数据已有，只是前端没显示）
- **测试功能不足**：缺泵运行计时、安全保护（干转/超时/日限额）测试引导、ppl 标定辅助
- **无联调步骤引导**：硬件联调没有顺序化指引
- **轮询无异常处理**：fetch 失败会中断刷新

目标：增强现有 dashboard，覆盖"测试所有功能、展示所有状态"。

## 2. 决策记录（用户确认）

| 决策点 | 结论 |
|---|---|
| 页面形态 | 增强现有 dashboard（`/`），**不新建独立页面** |
| 状态展示新增 | 传感器有效性、编译特性/板型、I2C 设备自动显示、流量计实时刷新、安全参数当前值 |
| 明确不加 | WiFi 详情 |
| 测试功能新增 | 泵运行计时、ppl 标定计算器、安全功能测试区、联调步骤引导 |
| 实现方案 | **方案 A**：纯前端增强 + 后端仅加 `features` 字段，不新增任何 API 端点 |

## 3. 现状盘点（勿破坏）

### 3.1 现有 dashboard 已实现（保持不动）

- 实时状态卡：板型/版本、运行状态/安全状态、泵、阀、队列长度、会话 ml、日流量、WiFi IP、分区湿度(ADC)
- 故障横幅 + "清除故障 · 恢复运行"按钮
- 测试控制台：泵 ON/OFF、开选中阀/阀全关、队列浇水、急停、停止/清故障、标定干/湿、立即采样、读流量/清零、I2C 扫描、选中区/全部自动开关
- 系统参数表单（盆数/ppl/max_run/daily_limit/dry_run）、分区配置表单、WiFi 设置表单

### 3.2 现有 API（全部复用，不加新端点）

| 方法/路径 | 用途 |
|---|---|
| GET `/api/status` | 实时状态（含 zones[].sensor_valid） |
| GET/POST `/api/settings` | 读写配置（含 max_run_sec/daily_limit_ml/dry_run_sec/ppl） |
| POST `/api/irrigate` | 队列浇水（**走控制器流程**） |
| POST `/api/emergency-stop` / `/api/stop` | 急停 / 停止+清故障 |
| POST `/api/test/pump` / `/api/test/valve` | 调试直通泵/阀（**绕过控制器**） |
| POST `/api/cal` | 湿度标定 dry/wet |
| GET `/api/flow` / POST `/api/flow/reset` | 流量读数 / 清零 |
| GET `/api/i2cscan` | I2C 扫描 |
| POST `/api/sample` | 立即采样 |
| POST `/api/auto` | 自动模式开关 |
| GET/POST `/api/wifi` | WiFi 读写 |

### 3.3 关键机制约束（设计依据）

- **干转/超时保护只在控制器开泵时生效**：`irrigation_controller.cpp` 的 `tick()` 在 `Pumping` 状态调用 `safety_->checkDryRun()` / `checkTimeout()`；而 `/api/test/pump` 直接调 `g_pump.set()` 绕过控制器 → **安全测试必须走"队列浇水"流程**，页面引导文案须写明
- `sensor_valid`（`zone_status[i].sensor_valid`）、安全参数均已由现有接口返回，前端仅需展示

## 4. 设计

### 4.1 后端改动（唯一一处）

`web_server.cpp` 的 `handleStatus()` 增加 `features` 对象：

```json
"features": {
  "valves":      true,   // = IRRIGATION_HAS_VALVES != 0
  "flow_meter":  true,   // = IRRIGATION_HAS_FLOW_METER != 0
  "max_zones":   10,     // = MAX_ZONES
  "max_valves":  10      // = BOARD_VALVE_COUNT
}
```

> 实现注意：`IRRIGATION_HAS_*` 宏是 int 0/1，必须用 `!= 0` 显式转成 bool，否则 ArduinoJson 会序列化为数字而非 `true`/`false`。

### 4.2 状态展示增强（"实时状态"卡片）

| 新增展示 | 数据来源 | 位置 |
|---|---|---|
| 板型 + 固件版本 + 编译特性（阀 ✓/流量计 ✓/最大阀数） | `features` | 状态卡顶部 |
| 安全参数当前值：max_run / daily_limit / dry_run | `/api/settings`（loadAll 已拉取） | 状态卡 |
| 传感器有效性：每盆行尾 `· 有效`/`· 无效`（无效标红） | `zones[].sensor_valid` | 分区列表每行 |
| I2C 设备自动显示：页面加载自动扫描并常驻，保留手动"重扫" | `/api/i2cscan` | 湿度/流量/I2C section |
| 流量计实时刷新：随轮询自动拉取脉冲/体积，替换"读流量"按钮 | `/api/flow` | 同上 |

### 4.3 测试功能增强（测试控制台）

**泵运行计时**：泵 ON 时显示 `泵已运行 N.N s`（前端记录 pump 变 true 的时刻，随轮询更新；页面刷新归零，标注"本次页面会话"）。直接"泵 ON"与浇水流程开泵均计时。

**ppl 标定计算器**（新 section）：

```
量杯体积 [500] ml  ×  当前脉冲 [自动填入，可手改]
→ 实时显示 ppl = 脉冲 × 1000 / 体积
[写入并保存] → POST /api/settings { pulses_per_liter }
提示文案：先"流量清零" → 浇已知体积 → 读脉冲 → 计算 → 写入
```

**安全功能测试区**（新 section，3 个引导测试，步骤/预期/恢复说明，可勾选并 localStorage 持久化）：

| 测试 | 操作 | 预期结果 |
|---|---|---|
| 干转保护 | 无水状态 → 队列浇水 50ml | `dry_run_sec`（默认 3s）内 FAULT/DRY_RUN，泵自停 → "停止/清故障"解锁 |
| 超时保护 | `max_run_sec` 临时设 5s 保存 → 队列浇水 | 泵运行 5s 后 FAULT/TIMEOUT → 恢复 `max_run_sec` |
| 日限额 | `daily_limit_ml` 临时设 100ml 保存 → 连续浇水 | 累计超限后状态 DAILY_LIMIT，自动模式停触发 → 恢复限额 |

> 页面须显著提示：**必须走"队列浇水"（控制器流程）才能触发保护**；直接"泵 ON"是调试直通，测不出干转/超时。

### 4.4 联调步骤引导（新卡片）

6 步可勾选清单（localStorage 持久化，当前步骤高亮，**不做自动判断**）：

`① I2C 扫描 → ② 泵测试 → ③ 阀测试 → ④ 流量计 → ⑤ 干转保护 → ⑥ 标定`

### 4.5 错误处理

- 给轮询（status/flow）加 try/catch：fetch 失败时状态卡显示"连接中断"，**继续重试不中断**（设备重启/WiFi 抖动时页面自愈）
- 浇水按钮不防重：队列天然处理重复请求，保持简单（YAGNI）

## 5. 明确不做（YAGNI）

- 不新建独立测试页面 `/test`
- 不新增任何后端 API 端点（`features` 字段除外）
- 不展示 WiFi 详情
- 不做浇水按钮防重
- 联调步骤不做自动完成判断（手动勾选）

## 6. 验证计划

1. `pio run -e esp32-s3-irrigation` 编译通过
2. 浏览器加载页面无 JS 报错，五个状态新增项正常显示
3. 硬件实测：泵 ON/OFF 与运行计时、I2C 自动显示、流量实时刷新、干转测试触发 DRY_RUN 并解锁恢复

## 7. 改动范围

- **仅 `src/web/web_server.cpp` 一个文件**（dashboard HTML/JS 增强 + `handleStatus` 加 `features` 字段），其余模块零触碰

## 8. 风险与缓解

| 风险 | 缓解 |
|---|---|
| 前端轮询请求增加（status 2s + flow 2s + 加载时 i2cscan 一次）可能加重 ESP32 单线程 WebServer 负载 | 低负载可承受；若页面卡顿，flow 轮询降频到 3–4s |
| localStorage 在嵌入式浏览器不可用 | 勾选状态降级为仅会话内有效，不阻塞功能 |
| 现有 dashboard 改动引入回归 | 保持既有 HTML 结构与类名，仅增量插入/追加；编译 + 浏览器验证 |
