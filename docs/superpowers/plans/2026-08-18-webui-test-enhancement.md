# Web UI 联调测试增强 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 增强现有 Web dashboard（`/`），补齐状态展示（编译特性/传感器有效性/I2C 自动显示/流量实时刷新/安全参数）与测试功能（泵计时/ppl 计算器/安全测试区/联调步骤引导），全部改动集中在 `src/web/web_server.cpp`。

**Architecture:** 纯前端（内嵌 HTML/JS 字符串）增强 + 后端 `/api/status` 增加一个 `features` 字段。复用现有全部 API 端点（`/api/settings`、`/api/flow`、`/api/i2cscan`、`/api/irrigate` 等），不新增端点。安全测试依赖现有控制器机制（干转/超时保护只在控制器开泵即"队列浇水"流程中生效）。

**Tech Stack:** ESP32-S3 / Arduino / PlatformIO / 单文件内嵌 Web UI（原生 JS + fetch + localStorage）。

**Spec:** `docs/superpowers/specs/2026-08-18-webui-test-enhancement-design.md`

**验证方式说明：** 本项目无单元测试框架（嵌入式）。每个代码任务的"测试"= `pio run -e esp32-s3-irrigation` 编译通过；最终以浏览器手动验证清单收尾。不为此引入测试框架（YAGNI）。

---

### Task 1: 后端 `/api/status` 增加 `features` 字段

**Files:**
- Modify: `src/web/web_server.cpp`（`handleStatus()`，约 484 行 `doc["session_ml"]` 之后）

- [ ] **Step 1: 定位插入点**

`handleStatus()` 内现有代码（锚点，勿改动内容）：

```cpp
  doc["daily_ml"] = ctx_->status->daily_ml;
  doc["active_zone"] = ctx_->status->active_zone;
  doc["queue"] = ctx_->status->queue_len;
  doc["session_ml"] = ctx_->status->session_ml;
```

- [ ] **Step 2: 在 `doc["session_ml"] = ctx_->status->session_ml;` 之后插入 features 块**

在 `handleStatus()` 中 `doc["session_ml"] = ctx_->status->session_ml;` 这一行**之后**追加：

```cpp

  JsonObject features = doc["features"].to<JsonObject>();
  features["valves"] = IRRIGATION_HAS_VALVES != 0;
  features["flow_meter"] = IRRIGATION_HAS_FLOW_METER != 0;
  features["max_zones"] = MAX_ZONES;
#if defined(BOARD_VALVE_COUNT)
  features["max_valves"] = BOARD_VALVE_COUNT;
#else
  features["max_valves"] = 0;
#endif
```

> 宏可见性已确认：`web_server.cpp` include 了 `config.h`，其中 `#include "platform.h"` 链入板头（`MAX_ZONES`、`BOARD_VALVE_COUNT`、`IRRIGATION_HAS_*` 全部可见）。`IRRIGATION_HAS_*` 是 int 0/1，必须 `!= 0` 转 bool，否则 ArduinoJson 序列化为数字而非 `true`/`false`。

- [ ] **Step 3: 编译验证**

Run: `cd /Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem && ~/.platformio/penv/bin/pio run -e esp32-s3-irrigation 2>&1 | tail -5`
Expected: 末尾 `SUCCESS`，无 error。

- [ ] **Step 4: Commit**

```bash
git add src/web/web_server.cpp
git commit -m "feat(web): /api/status 增加 features 编译特性字段"
```

---

### Task 2: dashboard HTML 结构增强

**Files:**
- Modify: `src/web/web_server.cpp`（`kDashboardHtml` 字符串，以下 4 处编辑按顺序执行）

- [ ] **Step 1: 样式表增加 `.step-current`**

锚点（现有）：

```css
.hint{font-size:.75rem;color:var(--muted);margin:-4px 0 8px}
```

替换为：

```css
.hint{font-size:.75rem;color:var(--muted);margin:-4px 0 8px}
.step-current{border:1px solid var(--accent);border-radius:6px;padding:4px 6px;background:rgba(0,204,136,.08)}
```

- [ ] **Step 2: 状态卡之后插入"联调步骤引导"卡片**

锚点（现有）：

```html
<div class="card" id="statusCard"><h2>实时状态</h2><div id="statusBody">加载中…</div></div>

<div class="card fault-banner" id="faultBanner" style="display:none">
```

替换为：

```html
<div class="card" id="statusCard"><h2>实时状态</h2><div id="statusBody">加载中…</div></div>

<div class="card"><h2>联调步骤引导</h2><p class="hint">按顺序逐项验证，完成勾选（本地保存）</p><div id="stepList"></div></div>

<div class="card fault-banner" id="faultBanner" style="display:none">
```

- [ ] **Step 3: 泵按钮区增加运行计时行**

锚点（现有）：

```html
<div class="btn-row btn-row3">
<button class="btn-ghost" onclick="doPump(1)">泵 ON</button>
<button class="btn-ghost" onclick="doPump(0)">泵 OFF</button>
<button class="btn-ghost" onclick="doValve(-1)">阀全关</button>
</div>
```

替换为（在原 `</div>` 之后追加计时行）：

```html
<div class="btn-row btn-row3">
<button class="btn-ghost" onclick="doPump(1)">泵 ON</button>
<button class="btn-ghost" onclick="doPump(0)">泵 OFF</button>
<button class="btn-ghost" onclick="doValve(-1)">阀全关</button>
</div>
<div class="row" id="pumpTimerRow" style="display:none"><span class="label">泵运行</span><span id="pumpTimer">—</span></div>
```

- [ ] **Step 4: "湿度 / 流量 / I2C" section 改造 + 追加 ppl 计算器与安全测试区**

锚点（现有，整段）：

```html
<div class="section"><div class="section-title">湿度 / 流量 / I2C</div>
<div class="btn-row btn-row3">
<button class="btn-ghost" onclick="doCal('dry')">标定 干</button>
<button class="btn-ghost" onclick="doCal('wet')">标定 湿</button>
<button class="btn-ghost" onclick="doSample()">立即采样</button>
</div>
<div class="btn-row btn-row3">
<button class="btn-ghost" onclick="doFlow()">读流量</button>
<button class="btn-ghost" onclick="doFlowReset()">流量清零</button>
<button class="btn-ghost" onclick="doI2cScan()">I2C 扫描</button>
</div>
<div class="row" id="flowInfo"><span class="label">流量计</span><span>—</span></div>
<div class="row" id="i2cInfo"><span class="label">I2C 设备</span><span>点击扫描</span></div>
</div>
```

替换为（删"读流量"按钮，改为自动刷新；追加两个新 section；注意外层测试控制台卡片的 `</div>` 在最后一行之后仍要闭合——本段替换保留 section 的 `</div>` 结构）：

```html
<div class="section"><div class="section-title">湿度 / 流量 / I2C</div>
<div class="btn-row btn-row3">
<button class="btn-ghost" onclick="doCal('dry')">标定 干</button>
<button class="btn-ghost" onclick="doCal('wet')">标定 湿</button>
<button class="btn-ghost" onclick="doSample()">立即采样</button>
</div>
<div class="btn-row">
<button class="btn-ghost" onclick="doFlowReset()">流量清零</button>
<button class="btn-ghost" onclick="doI2cScan()">I2C 重扫</button>
</div>
<div class="row" id="flowInfo"><span class="label">流量计</span><span>自动刷新…</span></div>
<div class="row" id="i2cInfo"><span class="label">I2C 设备</span><span>加载中…</span></div>
<div class="section"><div class="section-title">ppl 标定计算器</div>
<div class="grid2">
<div><label>量杯体积 (ml)</label><input type="number" id="calVol" value="500" min="10" oninput="calcPpl()"></div>
<div><label>当前脉冲</label><input type="number" id="calPulses" min="0" oninput="calcPpl()"></div>
</div>
<div class="row"><span class="label">计算 ppl</span><span id="calPpl">—</span></div>
<button class="btn-ghost" style="width:100%" onclick="applyPpl()">写入并保存 ppl</button>
<p class="hint">先"流量清零"→ 浇已知体积 → 脉冲自动填入 → 调整量杯体积 → 写入</p>
</div>
<div class="section"><div class="section-title">安全功能测试（须走"队列浇水"）</div>
<p class="hint">直接"泵 ON"绕过控制器，测不出保护；以下测试必须用"队列浇水"触发，完成后可勾选</p>
<div class="chk"><input type="checkbox" id="secDry" onchange="saveSecChk()"><span><b>干转保护</b>：无水时队列浇水 50ml → 约 3s 后 FAULT/DRY_RUN、泵自停 → 点"停止/清故障"解锁</span></div>
<div class="chk"><input type="checkbox" id="secTimeout" onchange="saveSecChk()"><span><b>超时保护</b>：系统参数 max_run 临时设 5s 保存 → 队列浇水 → 5s 后 FAULT/TIMEOUT → 恢复 max_run</span></div>
<div class="chk"><input type="checkbox" id="secDaily" onchange="saveSecChk()"><span><b>日限额</b>：系统参数日限额临时设 100ml 保存 → 连续浇水 → 超限后状态 DAILY_LIMIT → 恢复限额</span></div>
</div>
</div>
```

- [ ] **Step 5: Commit（HTML 尚未被 JS 引用，先提交结构）**

```bash
git add src/web/web_server.cpp
git commit -m "feat(web): dashboard 增加联调步骤/泵计时/ppl 计算/安全测试区 HTML 结构"
```

---

### Task 3: dashboard JS — 状态展示与自动轮询

**Files:**
- Modify: `src/web/web_server.cpp`（`<script>` 内，以下 4 处编辑）

- [ ] **Step 1: 全局变量增加 `pumpOnSince`**

锚点（现有）：

```js
const MAXZ=10;
let settings={};
```

替换为：

```js
const MAXZ=10;
let settings={};
let pumpOnSince=null;
```

- [ ] **Step 2: 重写 `refreshStatus()`（加 features/安全参数/有效性/泵计时/错误处理）**

锚点（现有整函数，`function refreshStatus(){` 到其闭合 `}`，含 `async` 前缀）：

```js
async function refreshStatus(){
  const d=await api('/api/status');
  const fault=d.fault||d.state==='FAULT';
  document.getElementById('faultBanner').style.display=fault?'block':'none';
  if(fault){
    document.getElementById('faultText').textContent=
      '原因: '+(d.safety||'?')+' · 排除问题后点击下方按钮解除锁定（等同串口 stop）';
  }
  document.getElementById('statusBody').innerHTML=
    '<div class="row"><span class="label">板型</span><span>'+esc(d.board)+' v'+esc(d.firmware||'?')+'</span></div>'+
    '<div class="row"><span class="label">状态</span><span>'+d.state+' / '+d.safety+(d.safety_locked?' (锁定)':'')+'</span></div>'+
    '<div class="row"><span class="label">泵</span><span>'+(d.pump?'ON':'OFF')+'</span></div>'+
    '<div class="row"><span class="label">阀</span><span>'+(d.valve_on?'Z'+d.active_valve:'OFF')+'</span></div>'+
    '<div class="row"><span class="label">队列</span><span>'+d.queue+'</span></div>'+
    '<div class="row"><span class="label">本次会话</span><span>'+(d.session_ml!=null?d.session_ml:0)+' ml</span></div>'+
    '<div class="row"><span class="label">今日流量</span><span>'+d.daily_ml+' ml</span></div>'+
    '<div class="row"><span class="label">WiFi</span><span>'+(d.wifi&&d.wifi.connected?d.wifi.ip:'未连接')+'</span></div>'+
    (d.zones||[]).map(z=>'<div class="row"><span class="label">'+esc(z.name)+'</span><span>'+z.moisture_pct+'% (ADC '+z.moisture_adc+')'+(z.auto_enabled?' ·自动':'')+'</span></div>').join('');
  document.getElementById('liveHint').textContent='实时刷新 · '+new Date().toLocaleTimeString();
}
```

替换为：

```js
async function refreshStatus(){
  let d=null;
  try{d=await api('/api/status')}catch(e){}
  const live=document.getElementById('liveHint');
  if(!d){
    document.getElementById('statusBody').innerHTML='<div class="row"><span class="label">连接</span><span style="color:var(--warn)">中断，自动重试中…</span></div>';
    live.textContent='连接中断 · '+new Date().toLocaleTimeString();
    return;
  }
  const f=d.features||{};
  const fault=d.fault||d.state==='FAULT';
  document.getElementById('faultBanner').style.display=fault?'block':'none';
  if(fault){
    document.getElementById('faultText').textContent=
      '原因: '+(d.safety||'?')+' · 排除问题后点击下方按钮解除锁定（等同串口 stop）';
  }
  if(d.pump&&pumpOnSince===null)pumpOnSince=Date.now();
  if(!d.pump)pumpOnSince=null;
  const ptr=document.getElementById('pumpTimerRow');
  if(ptr)ptr.style.display=d.pump?'flex':'none';
  document.getElementById('statusBody').innerHTML=
    '<div class="row"><span class="label">板型</span><span>'+esc(d.board)+' v'+esc(d.firmware||'?')+'</span></div>'+
    '<div class="row"><span class="label">编译特性</span><span>阀 '+(f.valves?'✓':'✗')+' · 流量计 '+(f.flow_meter?'✓':'✗')+' · 最大阀数 '+(f.max_valves!=null?f.max_valves:'?')+'</span></div>'+
    '<div class="row"><span class="label">状态</span><span>'+d.state+' / '+d.safety+(d.safety_locked?' (锁定)':'')+'</span></div>'+
    '<div class="row"><span class="label">泵</span><span>'+(d.pump?'ON':'OFF')+'</span></div>'+
    '<div class="row"><span class="label">阀</span><span>'+(d.valve_on?'Z'+d.active_valve:'OFF')+'</span></div>'+
    '<div class="row"><span class="label">队列</span><span>'+d.queue+'</span></div>'+
    '<div class="row"><span class="label">本次会话</span><span>'+(d.session_ml!=null?d.session_ml:0)+' ml</span></div>'+
    '<div class="row"><span class="label">今日流量</span><span>'+d.daily_ml+' ml</span></div>'+
    '<div class="row"><span class="label">安全参数</span><span>'+(settings&&settings.max_run_sec!=null?'max_run '+settings.max_run_sec+'s / daily '+settings.daily_limit_ml+'ml / dry_run '+settings.dry_run_sec+'s':'—')+'</span></div>'+
    '<div class="row"><span class="label">WiFi</span><span>'+(d.wifi&&d.wifi.connected?d.wifi.ip:'未连接')+'</span></div>'+
    (d.zones||[]).map(z=>'<div class="row"><span class="label">'+esc(z.name)+'</span><span>'+z.moisture_pct+'% (ADC '+z.moisture_adc+')'+(z.auto_enabled?' ·自动':'')+(z.sensor_valid?' ·<span style="color:var(--accent)">有效</span>':' ·<span style="color:var(--warn)">无效</span>')+'</span></div>').join('');
  live.textContent='实时刷新 · '+new Date().toLocaleTimeString();
}
```

> 说明：`settings` 由 `loadAll()` 赋值，页面加载后 2s 内即有值，之前显示 `—`（可接受）。`sensor_valid` 有效用 accent 绿、无效用 warn 红。

- [ ] **Step 3: `doFlow()` 改为自动拉取的 `fetchFlow()`，并修正 `doFlowReset()`**

锚点（现有）：

```js
async function doFlow(){
  const f=await api('/api/flow');
  document.getElementById('flowInfo').innerHTML='<span class="label">流量计</span><span>'+f.pulses+' 脉冲 · '+f.volume_ml+' ml (ppl '+f.pulses_per_liter+')</span>';
}
async function doFlowReset(){
  await api('/api/flow/reset',{method:'POST'});
  msg('流量计数已清零');doFlow();
}
```

替换为：

```js
async function fetchFlow(){
  let f=null;
  try{f=await api('/api/flow')}catch(e){}
  if(!f)return;
  document.getElementById('flowInfo').innerHTML='<span class="label">流量计</span><span>'+f.pulses+' 脉冲 · '+f.volume_ml+' ml (ppl '+f.pulses_per_liter+')</span>';
  const cp=document.getElementById('calPulses');
  if(cp&&document.activeElement!==cp)cp.value=f.pulses;
  calcPpl();
}
async function doFlowReset(){
  await api('/api/flow/reset',{method:'POST'});
  msg('流量计数已清零');fetchFlow();
}
```

- [ ] **Step 4: 初始化行加入自动扫描/轮询**

锚点（现有）：

```js
loadAll();refreshStatus();setInterval(refreshStatus,2000);
```

替换为：

```js
loadAll();refreshStatus();fetchFlow();setTimeout(doI2cScan,500);
loadSecChk();renderSteps();
setInterval(refreshStatus,2000);
setInterval(fetchFlow,2000);
setInterval(updatePumpTimer,500);
```

> 注意：`loadSecChk`/`renderSteps`/`updatePumpTimer`/`calcPpl` 在 Task 4 定义，Task 4 完成前本任务**不编译**（JS 在 PROGMEM 字符串内，未定义函数在编译期不报错，但避免半成品提交）。

- [ ] **Step 5: Commit**

```bash
git add src/web/web_server.cpp
git commit -m "feat(web): 状态展示增强与自动轮询（features/有效性/安全参数/流量/I2C/断线重试）"
```

---

### Task 4: dashboard JS — 测试功能（泵计时 / ppl / 安全勾选 / 步骤引导）

**Files:**
- Modify: `src/web/web_server.cpp`（`<script>` 内，插入 4 组函数）

- [ ] **Step 1: 插入泵计时函数**

锚点（现有，`doPump` 函数之后）：

```js
async function doPump(on){await api('/api/test/pump',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({on:!!on})});refreshStatus()}
```

在其后追加：

```js
function updatePumpTimer(){
  if(pumpOnSince){
    const t=document.getElementById('pumpTimer');
    if(t)t.textContent=((Date.now()-pumpOnSince)/1000).toFixed(1)+' s（本次页面会话）';
  }
}
```

- [ ] **Step 2: 插入 ppl 计算器函数**

锚点（现有，`doFlowReset` 函数之后——Task 3 已将其改写为调用 `fetchFlow()`）：

```js
async function doFlowReset(){
  await api('/api/flow/reset',{method:'POST'});
  msg('流量计数已清零');fetchFlow();
}
```

在其后追加：

```js
function calcPpl(){
  const v=+document.getElementById('calVol').value;
  const p=+document.getElementById('calPulses').value;
  document.getElementById('calPpl').textContent=(v>0&&p>0)?Math.round(p*1000/v):'—';
}
async function applyPpl(){
  const v=+document.getElementById('calVol').value;
  const p=+document.getElementById('calPulses').value;
  if(!(v>0&&p>0)){msg('请先填写量杯体积和脉冲数');return}
  const ppl=Math.round(p*1000/v);
  const r=await api('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({pulses_per_liter:ppl})});
  if(r&&r.ok){msg('ppl='+ppl+' 已保存');loadAll()}else msg('保存失败');
}
```

- [ ] **Step 3: 插入安全测试勾选函数（localStorage，key: `irr_sec`）**

锚点（现有，`doAutoAll` 函数之后）：

```js
async function doAutoAll(on){
  await api('/api/auto',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({all:true,enabled:!!on})});
  msg('全部自动'+(on?'开启':'关闭'));loadAll();refreshStatus();
}
```

在其后追加：

```js
let secChk={};
try{secChk=JSON.parse(localStorage.getItem('irr_sec')||'{}')}catch(e){}
function loadSecChk(){
  for(const k in secChk){
    const el=document.getElementById(k);
    if(el)el.checked=!!secChk[k];
  }
}
function saveSecChk(){
  ['secDry','secTimeout','secDaily'].forEach(id=>{secChk[id]=!!document.getElementById(id).checked});
  try{localStorage.setItem('irr_sec',JSON.stringify(secChk))}catch(e){}
}
```

- [ ] **Step 4: 插入联调步骤引导函数（localStorage，key: `irr_steps`）**

锚点（现有，`applyZoneCount` 函数之后）：

```js
async function applyZoneCount(){
  const n=Math.min(+document.getElementById('zoneCount').value||1,MAXZ);
  await api('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({zone_count:n})});
  msg('盆数='+n+' 已应用');loadAll();refreshStatus();
}
```

在其后追加：

```js
const STEPS=['I2C 扫描','泵测试','阀测试','流量计','干转保护','标定'];
let stepsDone={};
try{stepsDone=JSON.parse(localStorage.getItem('irr_steps')||'{}')}catch(e){}
function renderSteps(){
  const el=document.getElementById('stepList');
  if(!el)return;
  let h='';
  let curIdx=STEPS.findIndex((s,i)=>!stepsDone[i]);
  for(let i=0;i<STEPS.length;i++){
    const done=!!stepsDone[i];
    h+='<div class="chk'+(i===curIdx?' step-current':'')+'"><input type="checkbox" '+(done?'checked':'')+' onchange="toggleStep('+i+')"><span>'+(i+1)+'. '+STEPS[i]+(done?' ✓':'')+'</span></div>';
  }
  el.innerHTML=h;
}
function toggleStep(i){
  const cb=document.querySelectorAll('#stepList input')[i];
  stepsDone[i]=!!cb.checked;
  try{localStorage.setItem('irr_steps',JSON.stringify(stepsDone))}catch(e){}
  renderSteps();
}
```

- [ ] **Step 5: 编译验证**

Run: `cd /Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem && ~/.platformio/penv/bin/pio run -e esp32-s3-irrigation 2>&1 | tail -5`
Expected: 末尾 `SUCCESS`，无 error。

- [ ] **Step 6: Commit**

```bash
git add src/web/web_server.cpp
git commit -m "feat(web): 测试控制台新增泵计时/ppl 计算器/安全测试勾选/联调步骤引导"
```

---

### Task 5: 最终编译 + 手动验证

**Files:**
- 无代码改动

- [ ] **Step 1: 全量编译**

Run: `cd /Users/yanx/ESP32/AutoIrrigationSystem/AutoIrrigationSystem && ~/.platformio/penv/bin/pio run -e esp32-s3-irrigation 2>&1 | tail -8`
Expected: `RAM: ~15%`、`Flash: ~25%`、末尾 `SUCCESS`。

- [ ] **Step 2: 烧录**

Run: `~/.platformio/penv/bin/pio run -e esp32-s3-irrigation -t upload`

- [ ] **Step 3: 浏览器手动验证清单（连接设备 IP 或 `192.168.4.1`）**

| # | 检查项 | 预期 |
|---|---|---|
| 1 | 实时状态卡 | 出现"编译特性"行：阀 ✓ · 流量计 ✓ · 最大阀数 10 |
| 2 | 实时状态卡 | 出现"安全参数"行：max_run / daily / dry_run 生效值 |
| 3 | 分区列表 | 每盆行尾有"有效"（绿）/"无效"（红）标记 |
| 4 | I2C 行 | 页面加载 0.5s 后自动显示扫描结果（0x20 0x48 0x49 0x4A，按实际接线） |
| 5 | 流量行 | 每 2s 自动刷新脉冲/体积；"流量清零"按钮生效 |
| 6 | 泵 ON | 出现"泵运行 N.N s（本次页面会话）"并持续增长；泵 OFF 后消失 |
| 7 | ppl 计算器 | 输入体积/脉冲 → 实时算 ppl；"写入并保存 ppl"后系统参数区 ppl 更新 |
| 8 | 安全测试区 | 干转：无水队列浇水 50ml → 状态 FAULT/DRY_RUN → "停止/清故障"恢复；勾选持久化（刷新页面仍在） |
| 9 | 联调步骤引导 | 6 步清单显示，当前未完成步骤高亮，勾选后持久化 |
| 10 | 断线自愈 | 设备断电 10s → 状态卡显示"连接中断，自动重试中…"；设备恢复 → 自动恢复显示 |

- [ ] **Step 4: 无设备时的降级验证（可选）**

若硬件不在手边，仅执行 Task 5 Step 1 编译 + 用浏览器打开现有部署固件的页面确认无 JS 语法错误（F12 console 无红错）即可，硬件项留待上机。

---

### Task 6: 文档同步（DOC_MAP 要求）

**Files:**
- Modify: `docs/development.md`（Web 章节补充测试台说明）
- Modify: `docs/DOC_MAP.md`（版本记录追加一行）

- [ ] **Step 1: development.md 增加 dashboard 联调能力说明**

定位 `docs/development.md` 第 5 节"Web API"开头的 `Base URL: http://<device-ip>/` 行，在其后追加一段：

```markdown
Dashboard（`/`）内嵌联调测试能力（2026-08-18 增强）：
- 实时状态含：编译特性（阀/流量计/最大阀数）、安全参数生效值、各盆传感器有效性
- I2C 设备列表与流量计读数自动刷新（2s），无需手动点击
- 测试控制台含：泵运行计时、ppl 标定计算器（量杯体积+脉冲 → 写入）、安全功能测试引导（干转/超时/日限额，须走"队列浇水"流程触发）、联调步骤引导（6 步勾选，localStorage 保存）
```

- [ ] **Step 2: DOC_MAP.md 版本记录追加**

锚点（现有最后一行）：

```markdown
| 2026-08-01 | C3 0.42 OLED 板：引脚改为 I2C 5/6、流量计 GPIO7；新增 board-esp32-c3-042-oled.md |
```

在其后追加：

```markdown
| 2026-08-18 | Web dashboard 联调测试增强：features 字段、状态展示、泵计时/ppl/安全测试/步骤引导 |
```

- [ ] **Step 3: Commit**

```bash
git add docs/development.md docs/DOC_MAP.md
git commit -m "docs: Web dashboard 联调测试增强（development/DOC_MAP）"
```

---

## Self-Review 记录（实现前已核对）

**Spec 覆盖：** 4.1 features → Task 1；4.2 五项状态展示 → Task 2/3（编译特性/安全参数/有效性 → refreshStatus；I2C 自动 → setTimeout(doI2cScan)；流量实时 → fetchFlow 轮询）；4.3 四项测试功能 → Task 2(HTML)+Task 4(JS)；4.4 步骤引导 → Task 2+4；4.5 错误处理 → refreshStatus/fetchFlow try/catch。无缺口。

**占位符扫描：** 无 TBD/TODO；所有代码块完整可粘贴；所有锚点为现有文件真实文本（已从 web_server.cpp 逐行核对）。

**类型/命名一致性：** `pumpOnSince`/`updatePumpTimer`/`pumpTimerRow`/`pumpTimer` 全链一致；`fetchFlow`/`calcPpl`/`applyPpl`/`calVol`/`calPulses`/`calPpl` 一致；`secChk`/`loadSecChk`/`saveSecChk`/`secDry`/`secTimeout`/`secDaily` 一致；`stepsDone`/`renderSteps`/`toggleStep`/`stepList`/`STEPS` 一致。Task 4 定义的 `loadSecChk`/`renderSteps`/`updatePumpTimer` 恰为 Task 3 Step 4 初始化行所引用（JS 运行时解析，顺序正确）。

**已知取舍（实现时勿“改进”）：** 泵计时为页面会话级（刷新归零）；安全参数行在 loadAll 完成前显示 `—`；不新增 API 端点；不做浇水防重。
