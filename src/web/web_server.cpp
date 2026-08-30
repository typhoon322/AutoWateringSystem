#include "web/web_server.h"

#include <ArduinoJson.h>
#include <WebServer.h>
#include <WiFi.h>

#include "actuator/pump_driver.h"
#include "actuator/valve_driver.h"
#include "config.h"
#include "control/irrigation_controller.h"
#include "control/zone_manager.h"
#include "safety/safety_monitor.h"
#include "safety/selfcheck.h"
#include "sensor/flow_meter.h"
#include "storage/settings_store.h"
#include "storage/irrigation_history.h"
#include "bus/i2c_bus.h"

extern SettingsStore g_settings;
extern PumpDriver g_pump;
extern ValveDriver g_valves;
extern FlowMeter g_flow;
extern ZoneManager g_zone_manager;
extern SafetyMonitor g_safety;
extern void purgeSet(bool on);  // main.cpp 排气模式
#include "control/stress_test.h"  // g_stress 稳定性测试

namespace {
WebServer server(80);

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
<div class="card" id="scWarn" style="display:none;border-color:var(--warn);color:var(--warn);font-size:.85rem;padding:10px 14px"></div>
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
function esc(s){return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/"/g,'&quot;').replace(/'/g,'&#39;')}
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
      '<div><span class="zone-name">🪴 '+esc(z.name||('Zone'+i))+'</span><br><span class="'+c+'" id="zs'+i+'"><span class="status-dot"></span>'+pctLabel(c)+'</span></div>'+
      '<div class="zone-pct '+c+'" id="zp'+i+'">'+pct+'%</div></div>'+
      '<div class="zone-actions" id="za'+i+'">'+
      '<div class="row"><span>自动浇水</span><button class="switch'+(z.auto_enabled?' on':'')+'" onclick="toggleAuto('+i+')"></button></div>'+
      '<div class="row"><label>水量 ml</label><input type="number" id="vol'+i+'" value="'+(z.volume_ml!=null&&z.volume_ml>0?z.volume_ml:100)+'" min="10"></div>'+
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
  const sc=document.getElementById('scWarn');
  if(sc){
    if(d.selfcheck&&d.selfcheck.warnings>0){
      sc.style.display='block';
      sc.textContent='⚠️ 启动自检警告：'+(d.selfcheck.pca9555?'':' 阀扩展板缺失')+(d.selfcheck.ads_ok?'':' 湿度板缺失')+(d.selfcheck.oled?'':' 屏幕缺失')+'（详情见调试页）';
    }else{
      sc.style.display='none';
    }
  }
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
  const zc=document.getElementById('zonesCard');
  if(zc&&d.zones){
    for(let i=0;i<d.zones.length;i++){
      const z=d.zones[i]||{};
      const pctEl=document.getElementById('zp'+i);
      if(pctEl){
        const c=pctColor(z);
        pctEl.textContent=(z.moisture_pct!=null?z.moisture_pct:'--')+'%';
        pctEl.className='zone-pct '+c;
        const st=document.getElementById('zs'+i);
        if(st){st.className=c;st.innerHTML='<span class="status-dot"></span>'+pctLabel(c)}
      }
    }
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
    const n=((names[r.zone]||{}).name)||('盆'+(r.zone+1));
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

const char kDevHtml[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>AutoIrrigation · 调试</title>
<style>
:root{--bg:#0f1419;--card:#1a2332;--accent:#00cc88;--warn:#ff5555;--text:#e6edf3;--muted:#8b949e;--blue:#388bfd}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,sans-serif;background:var(--bg);color:var(--text);padding:12px;line-height:1.4}
.wrap{max-width:560px;margin:0 auto}
h1{color:var(--accent);font-size:1.15rem;margin-bottom:10px}
h2{font-size:.95rem;color:var(--accent);margin:0 0 8px}
.card{background:var(--card);border-radius:10px;padding:14px;margin-bottom:10px;border:1px solid #30363d}
.row{display:flex;justify-content:space-between;gap:8px;margin:4px 0;font-size:.85rem}
.label{color:var(--muted);flex-shrink:0}
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:8px}
label{font-size:.75rem;color:var(--muted);display:block;margin-top:6px}
input,select{width:100%;padding:8px;margin:2px 0 4px;border:1px solid #30363d;border-radius:6px;background:#0d1117;color:var(--text);font-size:.85rem}
button{padding:10px;border:none;border-radius:6px;font-weight:600;cursor:pointer;font-size:.85rem}
.btn-row{display:grid;grid-template-columns:1fr 1fr;gap:6px;margin-top:8px}
.btn-row3{grid-template-columns:1fr 1fr 1fr}
.btn-go{background:var(--accent);color:#0f1419}
.btn-stop,.btn-warn{background:var(--warn);color:#fff}
.btn-save{background:var(--blue);color:#fff}
.btn-ghost{background:#21262d;color:var(--text);border:1px solid #30363d}
.zone-card{border:1px solid #30363d;border-radius:8px;padding:10px;margin-bottom:8px;background:#0d1117}
.zone-title{font-weight:700;color:var(--accent);margin-bottom:6px}
.chk{display:flex;align-items:center;gap:6px;margin:4px 0;font-size:.85rem}
.chk input{width:auto;margin:0}
.msg{text-align:center;font-size:.8rem;color:var(--muted);padding:8px 0;min-height:1.2em}
.live{font-size:.8rem;color:var(--muted);text-align:right;margin-bottom:6px}
.fault-banner{border-color:var(--warn)!important;background:#2a1515}
.fault-banner h2{color:var(--warn)}
.fault-banner p{font-size:.85rem;margin:6px 0 10px;color:#ffb4b4}
.hint{font-size:.75rem;color:var(--muted);margin:-4px 0 8px}
.step-current{border:1px solid var(--accent);border-radius:6px;padding:4px 6px;background:rgba(0,204,136,.08)}
.section{margin-top:6px;padding-top:8px;border-top:1px solid #30363d}
.section-title{font-size:.8rem;color:var(--muted);margin-bottom:6px}
</style>
</head>
<body>
<div class="wrap">
<h1>AutoIrrigation · 调试</h1>
<p class="hint"><a href="/" style="color:var(--accent)">← 返回主页</a> · 联调/开发用，家人用主页即可</p>
<div class="live" id="liveHint">刷新中…</div>

<div class="card" id="statusCard"><h2>实时状态</h2><div id="statusBody">加载中…</div></div>

<div class="card"><h2>联调步骤引导</h2><p class="hint">按顺序逐项验证，完成勾选（本地保存）</p><div id="stepList"></div></div>

<div class="card fault-banner" id="faultBanner" style="display:none">
<h2>故障锁定</h2>
<p id="faultText">系统已停止，需手动恢复。</p>
<button class="btn-go" style="width:100%" onclick="doStop()">清除故障 · 恢复运行</button>
</div>

<div class="card">
<h2>测试控制台</h2>
<p class="hint">等效串口 CLI：泵/阀/浇水/标定/流量/I2C，多数操作即时生效</p>
<div class="grid2">
<div><label>当前测试分区</label><select id="testZone"></select></div>
<div><label>手动体积 (ml)</label><input type="number" id="testMl" value="100" min="10" max="2000"></div>
</div>
<div class="section"><div class="section-title">浇水 / 安全</div>
<div class="btn-row btn-row3">
<button class="btn-go" onclick="doIrrigate()">队列浇水</button>
<button class="btn-warn" onclick="doEstop()">急停</button>
<button class="btn-ghost" onclick="doStop()">停止/清故障</button>
</div></div>
<div class="section"><div class="section-title">泵 / 阀（调试）</div>
<div class="btn-row btn-row3">
<button class="btn-ghost" onclick="doPump(1)">泵 ON</button>
<button class="btn-ghost" onclick="doPump(0)">泵 OFF</button>
<button class="btn-ghost" onclick="doValve(-1)">阀全关</button>
</div>
<div class="btn-row">
<button class="btn-ghost" onclick="doPurge(1)">排气 ON（阀全开+泵）</button>
<button class="btn-ghost" onclick="doPurge(0)">排气 OFF</button>
</div>
<div class="row" id="pumpTimerRow" style="display:none"><span class="label">泵运行</span><span id="pumpTimer">—</span></div>
<div class="btn-row">
<button class="btn-ghost" onclick="doValveSel()">开选中阀</button>
<button class="btn-ghost" onclick="doAuto(1)">选中区 自动ON</button>
<button class="btn-ghost" onclick="doAuto(0)">选中区 自动OFF</button>
</div>
<div class="btn-row">
<button class="btn-ghost" onclick="doAutoAll(1)">全部 自动ON</button>
<button class="btn-ghost" onclick="doAutoAll(0)">全部 自动OFF</button>
</div></div>
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
</div>

<div class="card">
<h2>系统参数</h2>
<div class="grid2">
<div><label>激活盆数 (1–10)</label><input type="number" id="zoneCount" min="1" max="10"></div>
<div><label>流量计 ppl</label><input type="number" id="ppl"></div>
<div><label>最长泵运行 (s)</label><input type="number" id="maxRun"></div>
<div><label>日限额 (ml)</label><input type="number" id="dailyLim"></div>
<div><label>干转判定 (s)</label><input type="number" id="dryRun"></div>
<div class="chk"><input type="checkbox" id="aWinEn"><span>自动浇水时段窗口</span></div>
<div><label>窗口开始 HH:MM</label><input type="time" id="aWinS"></div>
<div><label>窗口结束 HH:MM</label><input type="time" id="aWinE"></div>
<div class="section"><div class="section-title">稳定性测试（周期全盆浇水，验证历史记录）</div>
<div class="grid2">
<div><label>总时长 (小时)</label><input type="number" id="testDur" min="1" max="24"></div>
<div><label>间隔 (分钟)</label><input type="number" id="testInt" min="1"></div>
<div><label>每盆水量 (ml)</label><input type="number" id="testVol" min="5"></div>
<div><label>状态</label><span id="testState" style="font-size:.85rem">—</span></div>
</div>
<div class="btn-row">
<button class="btn-go" onclick="doStress(1)">启动测试</button>
<button class="btn-warn" onclick="doStress(0)">停止测试</button>
</div>
<div class="row" id="testInfo"><span class="label">统计</span><span>—</span></div>
</div>
</div>
<button class="btn-ghost" style="width:100%;margin-top:8px" onclick="applyZoneCount()">仅应用盆数（立即生效）</button>
</div>

<div class="card"><h2>分区配置</h2><div id="zoneForms"></div></div>

<div class="card">
<h2>WiFi</h2>
<label>SSID</label><input id="wifiSsid">
<label>密码</label><input id="wifiPass" type="password">
<div class="chk"><input type="checkbox" id="wifiEn"><span>启用 WiFi</span></div>
</div>

<button class="btn-save" style="width:100%" onclick="saveAll()">保存全部配置</button>
<p class="msg" id="msg"></p>
</div>
<script>
const MAXZ=10;
let settings={};
let pumpOnSince=null;
async function api(p,o){const r=await fetch(p,o);return r.json()}
function msg(t){document.getElementById('msg').textContent=t}
function esc(s){return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/"/g,'&quot;').replace(/'/g,'&#39;')}
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
function buildZoneForms(zc,zones){
  const n=Math.min(zc,MAXZ);
  let h='';
  for(let i=0;i<n;i++){
    const z=zones[i]||{};
    h+='<div class="zone-card"><div class="zone-title">分区 '+i+' · 实时 '+(z.moisture_pct!=null?z.moisture_pct+'%':'—')+'</div>'+
      '<label>名称</label><input id="zN'+i+'" value="'+esc(z.name||('Zone'+i))+'">'+
      '<div class="grid2"><div><label>湿度下限 %</label><input type="number" id="zLo'+i+'" value="'+(z.moisture_low||30)+'"></div>'+
      '<div><label>湿度上限 %</label><input type="number" id="zHi'+i+'" value="'+(z.moisture_high||60)+'"></div></div>'+
      '<div class="grid2"><div><label>单次 ml</label><input type="number" id="zVol'+i+'" value="'+(z.volume_ml||100)+'"></div>'+
      '<div><label>定时 HH:MM</label><input id="zSch'+i+'" value="'+String(z.schedule_hour||8).padStart(2,'0')+':'+String(z.schedule_minute||0).padStart(2,'0')+'"></div></div>'+
      '<div class="grid2"><div><label>覆盖窗口 开始</label><input type="time" id="zWS'+i+'"></div>'+
      '<div><label>覆盖窗口 结束</label><input type="time" id="zWE'+i+'"></div></div>'+
      '<div class="chk"><input type="checkbox" id="zWO'+i+'"><span>使用覆盖窗口（否则跟随全局）</span></div>'+
      '<div class="grid2"><div><label>校准 干 ADC</label><input type="number" id="zDry'+i+'" value="'+(z.cal_dry||3200)+'"></div>'+
      '<div><label>校准 湿 ADC</label><input type="number" id="zWet'+i+'" value="'+(z.cal_wet||1400)+'"></div></div>'+
      '<div class="chk"><input type="checkbox" id="zAuto'+i+'" '+(z.auto_enabled?'checked':'')+'><span>阈值自动</span></div>'+
      '<div class="chk"><input type="checkbox" id="zSchE'+i+'" '+(z.schedule_enabled?'checked':'')+'><span>定时浇水</span></div></div>';
  }
  document.getElementById('zoneForms').innerHTML=h;
  for(let i=0;i<n;i++){
    const z=zones[i]||{};
    document.getElementById('zWO'+i).checked=!!z.window_override;
    document.getElementById('zWS'+i).value=String(z.win_sh!=null?z.win_sh:17).padStart(2,'0')+':'+String(z.win_sm!=null?z.win_sm:0).padStart(2,'0');
    document.getElementById('zWE'+i).value=String(z.win_eh!=null?z.win_eh:21).padStart(2,'0')+':'+String(z.win_em!=null?z.win_em:0).padStart(2,'0');
  }
  const sel=document.getElementById('testZone');
  sel.innerHTML='';
  for(let i=0;i<n;i++)sel.innerHTML+='<option value="'+i+'">'+(zones[i]&&zones[i].name||('Zone'+i))+'</option>';
}
async function loadAll(){
  const [st,s,w]=await Promise.all([api('/api/status'),api('/api/settings'),api('/api/wifi')]);
  settings=s;
  document.getElementById('zoneCount').value=s.zone_count||1;
  document.getElementById('ppl').value=s.pulses_per_liter||450;
  document.getElementById('maxRun').value=s.max_run_sec||60;
  document.getElementById('dailyLim').value=s.daily_limit_ml||2000;
  document.getElementById('dryRun').value=s.dry_run_sec||3;
  document.getElementById('aWinEn').checked=!!s.auto_window_enabled;
  document.getElementById('aWinS').value=String(s.auto_win_sh!=null?s.auto_win_sh:17).padStart(2,'0')+':'+String(s.auto_win_sm!=null?s.auto_win_sm:0).padStart(2,'0');
  document.getElementById('aWinE').value=String(s.auto_win_eh!=null?s.auto_win_eh:21).padStart(2,'0')+':'+String(s.auto_win_em!=null?s.auto_win_em:0).padStart(2,'0');
  document.getElementById('testDur').value=s.test_duration_h!=null?s.test_duration_h:8;
  document.getElementById('testInt').value=s.test_interval_min!=null?s.test_interval_min:10;
  document.getElementById('testVol').value=s.test_volume_ml!=null?s.test_volume_ml:20;
  document.getElementById('wifiSsid').value=w.ssid||'';
  document.getElementById('wifiPass').value='';
  document.getElementById('wifiEn').checked=!!w.enabled;
  const zones=(s.zones||[]).map((z,i)=>Object.assign({},z,st.zones&&st.zones[i]));
  buildZoneForms(s.zone_count||1,zones);
}
function collectSettings(){
  const n=Math.min(+document.getElementById('zoneCount').value||1,MAXZ);
  const body={
    zone_count:n,
    pulses_per_liter:+document.getElementById('ppl').value,
    max_run_sec:+document.getElementById('maxRun').value,
    daily_limit_ml:+document.getElementById('dailyLim').value,
    dry_run_sec:+document.getElementById('dryRun').value,
    auto_window_enabled:document.getElementById('aWinEn').checked,
    auto_win_sh:document.getElementById('aWinS').value.split(':')[0]===''?17:+document.getElementById('aWinS').value.split(':')[0],
    auto_win_sm:document.getElementById('aWinS').value.split(':')[1]===''?0:+document.getElementById('aWinS').value.split(':')[1],
    auto_win_eh:document.getElementById('aWinE').value.split(':')[0]===''?21:+document.getElementById('aWinE').value.split(':')[0],
    auto_win_em:document.getElementById('aWinE').value.split(':')[1]===''?0:+document.getElementById('aWinE').value.split(':')[1],
    zones:[]
  };
  for(let i=0;i<n;i++){
    const sch=(document.getElementById('zSch'+i).value||'8:0').split(':');
    body.zones.push({
      name:document.getElementById('zN'+i).value,
      moisture_low:+document.getElementById('zLo'+i).value,
      moisture_high:+document.getElementById('zHi'+i).value,
      volume_ml:+document.getElementById('zVol'+i).value,
      schedule_hour:+sch[0]||0,
      schedule_minute:+sch[1]||0,
      cal_dry:+document.getElementById('zDry'+i).value,
      cal_wet:+document.getElementById('zWet'+i).value,
      auto_enabled:document.getElementById('zAuto'+i).checked,
      schedule_enabled:document.getElementById('zSchE'+i).checked,
      window_override:document.getElementById('zWO'+i).checked,
      win_sh:document.getElementById('zWS'+i).value.split(':')[0]===''?17:+document.getElementById('zWS'+i).value.split(':')[0],
      win_sm:document.getElementById('zWS'+i).value.split(':')[1]===''?0:+document.getElementById('zWS'+i).value.split(':')[1],
      win_eh:document.getElementById('zWE'+i).value.split(':')[0]===''?21:+document.getElementById('zWE'+i).value.split(':')[0],
      win_em:document.getElementById('zWE'+i).value.split(':')[1]===''?0:+document.getElementById('zWE'+i).value.split(':')[1]
    });
  }
  return body;
}
async function saveAll(){
  const body=collectSettings();
  const r=await api('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
  if(r.ok){
    await api('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({ssid:document.getElementById('wifiSsid').value,password:document.getElementById('wifiPass').value,enabled:document.getElementById('wifiEn').checked})});
    msg('已保存');
    loadAll();
  }else msg('保存失败');
}
async function doIrrigate(){
  const zone=+document.getElementById('testZone').value;
  const volume_ml=+document.getElementById('testMl').value;
  await api('/api/irrigate',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({zone,volume_ml})});
  msg('已加入浇水队列');refreshStatus();
}
async function doEstop(){await api('/api/emergency-stop',{method:'POST'});msg('急停');refreshStatus()}
async function doStop(){await api('/api/stop',{method:'POST'});msg('故障已清除，可继续操作');refreshStatus()}
async function doPump(on){await api('/api/test/pump',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({on:!!on})});refreshStatus()}
async function doPurge(on){await api('/api/purge',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({on:!!on})});msg(on?'排气中：阀全开+泵（手动停）':'排气结束');refreshStatus()}
async function doStress(on){
  const body={enabled:!!on};
  if(on){
    body.duration_h=+document.getElementById('testDur').value||8;
    body.interval_min=+document.getElementById('testInt').value||10;
    body.volume_ml=+document.getElementById('testVol').value||20;
  }
  await api('/api/stress',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
  msg(on?'稳定性测试启动':'测试已停止');loadStress();
}
async function loadStress(){
  let r=null;
  try{r=await api('/api/stress')}catch(e){}
  const st=document.getElementById('testState');
  const inf=document.getElementById('testInfo');
  if(!r||!st||!inf)return;
  st.textContent=r.enabled?('运行中 · 已 '+r.run_min+' 分钟 / 剩余 '+r.remain_min+' 分钟'):'未运行';
  st.style.color=r.enabled?'var(--accent)':'var(--muted)';
  inf.innerHTML='<span class="label">统计</span><span>轮次 '+r.round+' · 入队成功 '+r.ok+' · 失败 '+r.fail+' · 累计 '+r.total_ml+' ml</span>';
}
function updatePumpTimer(){
  if(pumpOnSince){
    const t=document.getElementById('pumpTimer');
    if(t)t.textContent=((Date.now()-pumpOnSince)/1000).toFixed(1)+' s（本次页面会话）';
  }
}
async function doValve(z){await api('/api/test/valve',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({zone:z})});refreshStatus()}
function doValveSel(){doValve(+document.getElementById('testZone').value)}
async function doCal(point){
  const zone=+document.getElementById('testZone').value;
  const r=await api('/api/cal',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({zone,point})});
  msg(r.ok?'标定 '+point+' Z'+zone+' = '+r.adc:'标定失败');
  loadAll();
}
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
async function doSample(){
  await api('/api/sample',{method:'POST'});
  msg('已采样湿度');refreshStatus();
}
async function doI2cScan(){
  const r=await api('/api/i2cscan');
  const txt=(r.devices||[]).map(a=>'0x'+Number(a).toString(16).toUpperCase()).join(' ')||'无设备';
  document.getElementById('i2cInfo').innerHTML='<span class="label">I2C 设备</span><span>'+txt+'</span>';
  msg('I2C: '+txt);
}
async function doAuto(on){
  const zone=+document.getElementById('testZone').value;
  await api('/api/auto',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({zone,enabled:!!on})});
  msg('Z'+zone+' 自动'+(on?'开启':'关闭'));loadAll();refreshStatus();
}
async function doAutoAll(on){
  await api('/api/auto',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({all:true,enabled:!!on})});
  msg('全部自动'+(on?'开启':'关闭'));loadAll();refreshStatus();
}
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
async function applyZoneCount(){
  const n=Math.min(+document.getElementById('zoneCount').value||1,MAXZ);
  await api('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({zone_count:n})});
  msg('盆数='+n+' 已应用');loadAll();refreshStatus();
}
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
loadAll();refreshStatus();fetchFlow();setTimeout(doI2cScan,500);
loadSecChk();renderSteps();loadStress();
setInterval(refreshStatus,2000);
setInterval(fetchFlow,2000);
setInterval(updatePumpTimer,500);
setInterval(loadStress,10000);
</script>
</body>
</html>)rawliteral";
}  // namespace

void WebServerUi::applyZoneCount(uint8_t n) {
  if (ctx_ == nullptr || ctx_->config == nullptr) {
    return;
  }
  if (n < MIN_ZONE_COUNT) {
    n = MIN_ZONE_COUNT;
  }
  if (n > MAX_ZONES) {
    n = MAX_ZONES;
  }
#if defined(BOARD_VALVE_COUNT)
  if (n > BOARD_VALVE_COUNT) {
    n = BOARD_VALVE_COUNT;
  }
#endif
  ctx_->config->zone_count = n;
  g_zone_manager.setCount(n);
}

void WebServerUi::begin(SystemContextEx *ctx) {
  ctx_ = ctx;
  setupRoutes();
}

void WebServerUi::resolveWifiCredentials(char *ssid, size_t ssid_len, char *pass,
                                         size_t pass_len) const {
  if (ssid_len == 0 || pass_len == 0) {
    return;
  }
  const char *src_ssid = WIFI_SSID;
  const char *src_pass = WIFI_PASS;
  if (ctx_ != nullptr && ctx_->config != nullptr && ctx_->config->wifi_ssid[0] != '\0') {
    src_ssid = ctx_->config->wifi_ssid;
    src_pass = ctx_->config->wifi_pass;
  }
  strncpy(ssid, src_ssid, ssid_len - 1);
  strncpy(pass, src_pass, pass_len - 1);
  ssid[ssid_len - 1] = '\0';
  pass[pass_len - 1] = '\0';
}

void WebServerUi::startWiFi() {
  if (ctx_ == nullptr) {
    return;
  }

  if (!ctx_->config->wifi_enabled) {
    if (WIFI_AP_FALLBACK) {
      WiFi.mode(WIFI_AP);
      WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
      Serial.printf("AP: %s\n", WiFi.softAPIP().toString().c_str());
    }
    wifi_connect_pending_ = false;
    wifi_started_ = true;
    server.begin();
    return;
  }

  char ssid[33];
  char pass[65];
  resolveWifiCredentials(ssid, sizeof(ssid), pass, sizeof(pass));

#if WIFI_HYBRID_MODE
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  if (WIFI_AP_FALLBACK) {
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    Serial.printf("AP: %s (hybrid)\n", WiFi.softAPIP().toString().c_str());
  }
#else
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
#endif

  if (ssid[0] != '\0') {
    WiFi.begin(ssid, pass);
    wifi_connect_pending_ = true;
    wifi_connect_start_ms_ = millis();
    Serial.printf("WiFi connecting to %s...\n", ssid);
  } else if (WIFI_AP_FALLBACK && WiFi.getMode() != WIFI_AP) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    Serial.printf("AP: %s\n", WiFi.softAPIP().toString().c_str());
  }

  wifi_started_ = true;
  server.begin();
}

void WebServerUi::restartWiFi() {
  wifi_started_ = false;
  wifi_connect_pending_ = false;
  WiFi.disconnect(true);
  startWiFi();
}

void WebServerUi::tickWiFi() {
  if (!ctx_->config->wifi_enabled) {
    return;
  }

  if (wifi_connect_pending_) {
    if (WiFi.status() == WL_CONNECTED) {
      wifi_connect_pending_ = false;
      Serial.printf("WiFi connected: %s\n", WiFi.localIP().toString().c_str());
      return;
    }
    if (millis() - wifi_connect_start_ms_ > 15000) {
      wifi_connect_pending_ = false;
      Serial.println(F("WiFi STA timeout (AP still available)."));
    }
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  const uint32_t now = millis();
  if (now - last_wifi_attempt_ms_ < WIFI_RECONNECT_INTERVAL_MS) {
    return;
  }
  last_wifi_attempt_ms_ = now;

  char ssid[33];
  char pass[65];
  resolveWifiCredentials(ssid, sizeof(ssid), pass, sizeof(pass));
  if (ssid[0] == '\0') {
    return;
  }

  Serial.println(F("WiFi lost — reconnecting..."));
  WiFi.begin(ssid, pass);
  wifi_connect_pending_ = true;
  wifi_connect_start_ms_ = now;
}

void WebServerUi::handleRoot() { server.send_P(200, "text/html", kHomeHtml); }
void WebServerUi::handleDev() { server.send_P(200, "text/html", kDevHtml); }

namespace {

const char *safetyDetail(SafetyState state) {
  switch (state) {
    case SafetyState::Ok:
      return "OK";
    case SafetyState::Timeout:
      return "TIMEOUT";
    case SafetyState::DryRun:
      return "DRY_RUN";
    case SafetyState::DailyLimit:
      return "DAILY_LIMIT";
    case SafetyState::Locked:
      return "LOCKED";
    default:
      return "UNKNOWN";
  }
}

}  // namespace

void WebServerUi::handleStatus() {
  if (ctx_ == nullptr) {
    server.send(500, "application/json", "{}");
    return;
  }

  JsonDocument doc;
  doc["board"] = BOARD_NAME;
  doc["firmware"] = FIRMWARE_VERSION;
  doc["max_zones"] = MAX_ZONES;
#if defined(BOARD_VALVE_COUNT)
  doc["max_valves"] = BOARD_VALVE_COUNT;
#endif
  doc["pump"] = ctx_->status->pump_on;
  doc["valve_on"] = ctx_->status->valve_on;
  doc["active_valve"] = ctx_->status->active_valve;
  doc["state"] = ctx_->controller ? ctx_->controller->stateText() : "IDLE";
  doc["safety"] = safetyDetail(ctx_->status->safety);
  doc["fault"] = ctx_->status->state == IrrigationState::Fault;
  doc["safety_locked"] = g_safety.isLocked();
  doc["daily_ml"] = ctx_->status->daily_ml;
  doc["active_zone"] = ctx_->status->active_zone;
  doc["queue"] = ctx_->status->queue_len;
  doc["session_ml"] = ctx_->status->session_ml;
  doc["purge_on"] = ctx_->status->purge_on;

  JsonObject features = doc["features"].to<JsonObject>();
  features["valves"] = IRRIGATION_HAS_VALVES != 0;
  features["flow_meter"] = IRRIGATION_HAS_FLOW_METER != 0;
  features["max_zones"] = MAX_ZONES;
#if defined(BOARD_VALVE_COUNT)
  features["max_valves"] = BOARD_VALVE_COUNT;
#else
  features["max_valves"] = 0;
#endif

  JsonObject sc = doc["selfcheck"].to<JsonObject>();
  sc["pca9555"] = g_selfcheck.pca9555_ok;
  sc["ads_ok"] = g_selfcheck.ads_ok;
  sc["oled"] = g_selfcheck.oled_ok;
  sc["warnings"] = g_selfcheck.warnings;
  sc["done"] = g_selfcheck.done;

  JsonObject wifi = doc["wifi"].to<JsonObject>();
  wifi["enabled"] = ctx_->config->wifi_enabled;
  wifi["connected"] = WiFi.status() == WL_CONNECTED;
  wifi["ssid"] = ctx_->config->wifi_ssid[0] != '\0' ? ctx_->config->wifi_ssid : WIFI_SSID;
  if (ctx_->config->wifi_enabled && WiFi.getMode() == WIFI_AP_STA) {
    wifi["mode"] = "AP_STA";
    wifi["ap_ip"] = WiFi.softAPIP().toString();
    wifi["ip"] = WiFi.localIP().toString();
  } else if (WiFi.getMode() == WIFI_AP) {
    wifi["mode"] = "AP";
    wifi["ap_ip"] = WiFi.softAPIP().toString();
    wifi["ip"] = WiFi.softAPIP().toString();
  } else if (WiFi.status() == WL_CONNECTED) {
    wifi["mode"] = "STA";
    wifi["ip"] = WiFi.localIP().toString();
  } else {
    wifi["mode"] = ctx_->config->wifi_enabled ? "STA" : "OFF";
    wifi["ip"] = "";
  }
  wifi["rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;

  JsonArray zones = doc["zones"].to<JsonArray>();
  for (uint8_t i = 0; i < ctx_->config->zone_count; ++i) {
    JsonObject z = zones.add<JsonObject>();
    z["id"] = i;
    z["name"] = ctx_->zones[i].name;
    z["moisture_pct"] = ctx_->zone_status[i].moisture_pct;
    z["moisture_adc"] = ctx_->zone_status[i].moisture_adc;
    z["sensor_valid"] = ctx_->zone_status[i].sensor_valid;
    z["auto_enabled"] = ctx_->zones[i].auto_enabled;
    z["schedule_enabled"] = ctx_->zones[i].schedule_enabled;
    z["moisture_low"] = ctx_->zones[i].moisture_low;
    z["moisture_high"] = ctx_->zones[i].moisture_high;
    z["volume_ml"] = ctx_->zones[i].volume_ml;
    z["schedule_hour"] = ctx_->zones[i].schedule_hour;
    z["schedule_minute"] = ctx_->zones[i].schedule_minute;
    z["cal_dry"] = ctx_->zones[i].cal_dry;
    z["cal_wet"] = ctx_->zones[i].cal_wet;
  }

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void WebServerUi::handleSettingsGet() {
  if (ctx_ == nullptr) {
    server.send(500, "application/json", "{}");
    return;
  }

  JsonDocument doc;
  doc["max_run_sec"] = ctx_->config->max_run_sec;
  doc["daily_limit_ml"] = ctx_->config->daily_limit_ml;
  doc["pulses_per_liter"] = ctx_->config->pulses_per_liter;
  doc["dry_run_sec"] = ctx_->config->dry_run_sec;
  doc["zone_count"] = ctx_->config->zone_count;
  doc["auto_window_enabled"] = ctx_->config->auto_window_enabled;
  doc["auto_win_sh"] = ctx_->config->auto_win_sh;
  doc["auto_win_sm"] = ctx_->config->auto_win_sm;
  doc["auto_win_eh"] = ctx_->config->auto_win_eh;
  doc["auto_win_em"] = ctx_->config->auto_win_em;
  doc["max_zones"] = MAX_ZONES;
#if defined(BOARD_VALVE_COUNT)
  doc["max_valves"] = BOARD_VALVE_COUNT;
#endif

  JsonArray zones = doc["zones"].to<JsonArray>();
  for (uint8_t i = 0; i < ctx_->config->zone_count; ++i) {
    JsonObject z = zones.add<JsonObject>();
    z["name"] = ctx_->zones[i].name;
    z["moisture_low"] = ctx_->zones[i].moisture_low;
    z["moisture_high"] = ctx_->zones[i].moisture_high;
    z["volume_ml"] = ctx_->zones[i].volume_ml;
    z["auto_enabled"] = ctx_->zones[i].auto_enabled;
    z["schedule_enabled"] = ctx_->zones[i].schedule_enabled;
    z["schedule_hour"] = ctx_->zones[i].schedule_hour;
    z["schedule_minute"] = ctx_->zones[i].schedule_minute;
    z["cal_dry"] = ctx_->zones[i].cal_dry;
    z["cal_wet"] = ctx_->zones[i].cal_wet;
    z["window_override"] = ctx_->zones[i].window_override;
    z["win_sh"] = ctx_->zones[i].win_sh;
    z["win_sm"] = ctx_->zones[i].win_sm;
    z["win_eh"] = ctx_->zones[i].win_eh;
    z["win_em"] = ctx_->zones[i].win_em;
  }

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void WebServerUi::handleSettingsPost() {
  if (ctx_ == nullptr || !server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  if (!g_selfcheck.done) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"selfcheck\"}");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  if (doc["max_run_sec"].is<uint16_t>()) {
    ctx_->config->max_run_sec = doc["max_run_sec"];
  }
  if (doc["daily_limit_ml"].is<uint32_t>()) {
    ctx_->config->daily_limit_ml = doc["daily_limit_ml"];
  }
  if (doc["pulses_per_liter"].is<uint16_t>()) {
    ctx_->config->pulses_per_liter = doc["pulses_per_liter"];
  }
  if (doc["dry_run_sec"].is<uint8_t>()) {
    ctx_->config->dry_run_sec = doc["dry_run_sec"];
  }
  if (doc["zone_count"].is<uint8_t>()) {
    applyZoneCount(doc["zone_count"]);
  }
  if (doc["auto_window_enabled"].is<bool>()) {
    ctx_->config->auto_window_enabled = doc["auto_window_enabled"];
  }
  if (doc["auto_win_sh"].is<uint8_t>()) ctx_->config->auto_win_sh = doc["auto_win_sh"];
  if (doc["auto_win_sm"].is<uint8_t>()) ctx_->config->auto_win_sm = doc["auto_win_sm"];
  if (doc["auto_win_eh"].is<uint8_t>()) ctx_->config->auto_win_eh = doc["auto_win_eh"];
  if (doc["auto_win_em"].is<uint8_t>()) ctx_->config->auto_win_em = doc["auto_win_em"];

  if (doc["zones"].is<JsonArray>()) {
    JsonArray arr = doc["zones"].as<JsonArray>();
    const uint8_t limit = ctx_->config->zone_count;
    uint8_t i = 0;
    for (JsonObject z : arr) {
      if (i >= limit || i >= MAX_ZONES) {
        break;
      }
      if (z["name"].is<const char *>()) {
        strncpy(ctx_->zones[i].name, z["name"], sizeof(ctx_->zones[i].name) - 1);
        ctx_->zones[i].name[sizeof(ctx_->zones[i].name) - 1] = '\0';
      }
      if (z["moisture_low"].is<uint8_t>()) {
        ctx_->zones[i].moisture_low = z["moisture_low"];
      }
      if (z["moisture_high"].is<uint8_t>()) {
        ctx_->zones[i].moisture_high = z["moisture_high"];
      }
      if (z["volume_ml"].is<uint16_t>()) {
        ctx_->zones[i].volume_ml = z["volume_ml"];
      }
      if (z["auto_enabled"].is<bool>()) {
        ctx_->zones[i].auto_enabled = z["auto_enabled"];
      }
      if (z["schedule_enabled"].is<bool>()) {
        ctx_->zones[i].schedule_enabled = z["schedule_enabled"];
      }
      if (z["schedule_hour"].is<uint8_t>()) {
        ctx_->zones[i].schedule_hour = z["schedule_hour"];
      }
      if (z["schedule_minute"].is<uint8_t>()) {
        ctx_->zones[i].schedule_minute = z["schedule_minute"];
      }
      if (z["cal_dry"].is<uint16_t>()) {
        ctx_->zones[i].cal_dry = z["cal_dry"];
      }
      if (z["cal_wet"].is<uint16_t>()) {
        ctx_->zones[i].cal_wet = z["cal_wet"];
      }
      if (z["window_override"].is<bool>()) {
        ctx_->zones[i].window_override = z["window_override"];
      }
      if (z["win_sh"].is<uint8_t>()) ctx_->zones[i].win_sh = z["win_sh"];
      if (z["win_sm"].is<uint8_t>()) ctx_->zones[i].win_sm = z["win_sm"];
      if (z["win_eh"].is<uint8_t>()) ctx_->zones[i].win_eh = z["win_eh"];
      if (z["win_em"].is<uint8_t>()) ctx_->zones[i].win_em = z["win_em"];
      ++i;
    }
  }

  SystemContext sc = {ctx_->config, ctx_->zones, ctx_->zone_status, ctx_->status};
  g_settings.save(sc);
  server.send(200, "application/json", "{\"ok\":true}");
}

void WebServerUi::handleIrrigate() {
  if (ctx_ == nullptr || !server.hasArg("plain") || ctx_->controller == nullptr) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  if (!g_selfcheck.done) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"selfcheck\"}");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  const uint8_t zone = doc["zone"] | 0;
  const uint16_t volume_ml = doc["volume_ml"] | DEFAULT_VOLUME_ML;
  const bool ok = ctx_->controller->requestManual(zone, volume_ml);
  server.send(ok ? 200 : 409, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

void WebServerUi::handleEmergencyStop() {
  if (ctx_ != nullptr && ctx_->controller != nullptr) {
    ctx_->controller->emergencyStop();
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void WebServerUi::handleStop() {
  if (ctx_ != nullptr && ctx_->controller != nullptr) {
    ctx_->controller->stop();
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void WebServerUi::handleTestPump() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  if (!g_selfcheck.done) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"selfcheck\"}");
    return;
  }
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  const bool on = doc["on"] | false;
  g_pump.set(on);
  server.send(200, "application/json", "{\"ok\":true}");
}

void WebServerUi::handleTestValve() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  if (!g_selfcheck.done) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"selfcheck\"}");
    return;
  }
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  if (doc["zone"].is<int>() && doc["zone"].as<int>() >= 0) {
    const uint8_t zone = doc["zone"].as<uint8_t>();
    if (ctx_ != nullptr && zone < ctx_->config->zone_count) {
      g_valves.open(zone);
    } else {
      server.send(400, "application/json", "{\"ok\":false}");
      return;
    }
  } else {
    g_valves.closeAll();
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void WebServerUi::handleCal() {
  if (ctx_ == nullptr || !server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  if (!g_selfcheck.done) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"selfcheck\"}");
    return;
  }
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  const uint8_t zone = doc["zone"] | 0;
  const char *point = doc["point"] | "";
  if (zone >= ctx_->config->zone_count) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  const uint16_t adc = ctx_->zone_status[zone].moisture_adc;
  if (strcmp(point, "dry") == 0) {
    ctx_->zones[zone].cal_dry = adc;
  } else if (strcmp(point, "wet") == 0) {
    ctx_->zones[zone].cal_wet = adc;
  } else {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  SystemContext sc = {ctx_->config, ctx_->zones, ctx_->zone_status, ctx_->status};
  g_settings.save(sc);

  JsonDocument out;
  out["ok"] = true;
  out["adc"] = adc;
  String s;
  serializeJson(out, s);
  server.send(200, "application/json", s);
}

void WebServerUi::handleFlow() {
  JsonDocument doc;
  doc["pulses"] = g_flow.pulses();
  if (ctx_ != nullptr && ctx_->config != nullptr) {
    doc["volume_ml"] = g_flow.volumeMl(ctx_->config->pulses_per_liter);
    doc["pulses_per_liter"] = ctx_->config->pulses_per_liter;
  }
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void WebServerUi::handleFlowReset() {
  if (!g_selfcheck.done) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"selfcheck\"}");
    return;
  }
  g_flow.resetSession();
  server.send(200, "application/json", "{\"ok\":true}");
}

void WebServerUi::handleI2cScan() {
  JsonDocument doc;
  JsonArray devices = doc["devices"].to<JsonArray>();
  doc["sda"] = PIN_I2C_SDA;
  doc["scl"] = PIN_I2C_SCL;
  for (uint8_t addr = 1; addr < 127; ++addr) {
    if (irrigationI2cProbe(addr)) {
      devices.add(addr);
    }
  }
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void WebServerUi::handleSample() {
  if (!g_selfcheck.done) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"selfcheck\"}");
    return;
  }
  g_zone_manager.sampleAll();
  server.send(200, "application/json", "{\"ok\":true}");
}

void WebServerUi::handleAuto() {
  if (ctx_ == nullptr || !server.hasArg("plain") || ctx_->zones == nullptr) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  if (!g_selfcheck.done) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"selfcheck\"}");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  const bool enabled = doc["enabled"] | false;
  if (doc["all"] | false) {
    for (uint8_t i = 0; i < ctx_->config->zone_count; ++i) {
      ctx_->zones[i].auto_enabled = enabled;
    }
  } else {
    const uint8_t zone = doc["zone"] | 0;
    if (zone >= ctx_->config->zone_count) {
      server.send(400, "application/json", "{\"ok\":false}");
      return;
    }
    ctx_->zones[zone].auto_enabled = enabled;
  }

  SystemContext sc = {ctx_->config, ctx_->zones, ctx_->zone_status, ctx_->status};
  g_settings.save(sc);
  server.send(200, "application/json", "{\"ok\":true}");
}

void WebServerUi::handleWifiGet() {
  JsonDocument doc;
  if (ctx_ != nullptr) {
    doc["enabled"] = ctx_->config->wifi_enabled;
    doc["ssid"] = ctx_->config->wifi_ssid[0] != '\0' ? ctx_->config->wifi_ssid : WIFI_SSID;
  }
  doc["connected"] = WiFi.status() == WL_CONNECTED;
  doc["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  if (WiFi.getMode() == WIFI_AP_STA) {
    doc["mode"] = "AP_STA";
    doc["ap_ip"] = WiFi.softAPIP().toString();
  } else if (WiFi.getMode() == WIFI_AP) {
    doc["mode"] = "AP";
  } else if (WiFi.status() == WL_CONNECTED) {
    doc["mode"] = "STA";
  } else {
    doc["mode"] = "OFF";
  }
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void WebServerUi::handleWifiPost() {
  if (ctx_ == nullptr || !server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  if (!g_selfcheck.done) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"selfcheck\"}");
    return;
  }  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  if (doc["ssid"].is<const char *>()) {
    strncpy(ctx_->config->wifi_ssid, doc["ssid"], sizeof(ctx_->config->wifi_ssid) - 1);
  }
  if (doc["password"].is<const char *>()) {
    strncpy(ctx_->config->wifi_pass, doc["password"], sizeof(ctx_->config->wifi_pass) - 1);
  }
  if (doc["enabled"].is<bool>()) {
    ctx_->config->wifi_enabled = doc["enabled"];
  }

  SystemContext sc = {ctx_->config, ctx_->zones, ctx_->zone_status, ctx_->status};
  g_settings.save(sc);
  restartWiFi();
  server.send(200, "application/json", "{\"ok\":true}");
}

// 排气模式：阀全开 + 泵直通（绕过控制器，不触发干转）；手动停
void WebServerUi::handlePurge() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  if (!g_selfcheck.done) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"selfcheck\"}");
    return;
  }
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  purgeSet(doc["on"] | false);
  server.send(200, "application/json", "{\"ok\":true}");
}

// 稳定性测试：GET 状态 / POST 配置与启停
void WebServerUi::handleStress() {
  if (ctx_ == nullptr) {
    server.send(500, "application/json", "{}");
    return;
  }
  if (server.method() == HTTP_GET) {
    JsonDocument doc;
    doc["enabled"] = g_stress.enabled();
    doc["duration_h"] = ctx_->config->test_duration_h;
    doc["interval_min"] = ctx_->config->test_interval_min;
    doc["volume_ml"] = ctx_->config->test_volume_ml;
    doc["round"] = g_stress.roundCount();
    doc["ok"] = g_stress.okCount();
    doc["fail"] = g_stress.failCount();
    doc["total_ml"] = g_stress.totalMl();
    if (g_stress.enabled()) {
      const uint32_t run_ms = millis() - g_stress.startMs();
      doc["run_min"] = run_ms / 60000U;
      doc["remain_min"] = static_cast<uint32_t>(ctx_->config->test_duration_h) * 60U -
                          run_ms / 60000U;
    }
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
    return;
  }
  // POST
  if (!g_selfcheck.done || !server.hasArg("plain")) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"selfcheck\"}");
    return;
  }
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  if (doc["duration_h"].is<uint8_t>()) {
    ctx_->config->test_duration_h = doc["duration_h"];
  }
  if (doc["interval_min"].is<uint8_t>()) {
    ctx_->config->test_interval_min = doc["interval_min"];
  }
  if (doc["volume_ml"].is<uint16_t>()) {
    ctx_->config->test_volume_ml = doc["volume_ml"];
  }
  if (doc["enabled"].is<bool>()) {
    g_stress.setEnabled(doc["enabled"]);
  }
  SystemContext sc = {ctx_->config, ctx_->zones, ctx_->zone_status, ctx_->status};
  g_settings.save(sc);
  server.send(200, "application/json", "{\"ok\":true}");
}

namespace {
const char *triggerText(uint8_t t) {
  switch (t) {
    case 1: return "threshold";
    case 2: return "schedule";
    case 3: return "manual";
    case 4: return "test";
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

void WebServerUi::setupRoutes() {
  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/dev", HTTP_GET, [this]() { handleDev(); });
  server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  server.on("/api/settings", HTTP_GET, [this]() { handleSettingsGet(); });
  server.on("/api/settings", HTTP_POST, [this]() { handleSettingsPost(); });
  server.on("/api/irrigate", HTTP_POST, [this]() { handleIrrigate(); });
  server.on("/api/emergency-stop", HTTP_POST, [this]() { handleEmergencyStop(); });
  server.on("/api/stop", HTTP_POST, [this]() { handleStop(); });
  server.on("/api/test/pump", HTTP_POST, [this]() { handleTestPump(); });
  server.on("/api/test/valve", HTTP_POST, [this]() { handleTestValve(); });
  server.on("/api/cal", HTTP_POST, [this]() { handleCal(); });
  server.on("/api/flow", HTTP_GET, [this]() { handleFlow(); });
  server.on("/api/flow/reset", HTTP_POST, [this]() { handleFlowReset(); });
  server.on("/api/i2cscan", HTTP_GET, [this]() { handleI2cScan(); });
  server.on("/api/sample", HTTP_POST, [this]() { handleSample(); });
  server.on("/api/auto", HTTP_POST, [this]() { handleAuto(); });
  server.on("/api/wifi", HTTP_GET, [this]() { handleWifiGet(); });
  server.on("/api/wifi", HTTP_POST, [this]() { handleWifiPost(); });
  server.on("/api/purge", HTTP_POST, [this]() { handlePurge(); });
  server.on("/api/stress", HTTP_GET, [this]() { handleStress(); });
  server.on("/api/stress", HTTP_POST, [this]() { handleStress(); });
  server.on("/api/history", HTTP_GET, [this]() { handleHistory(); });
}

void WebServerUi::loop() {
  if (!wifi_started_) {
    startWiFi();
  }
  server.handleClient();

  if (ctx_ != nullptr && ctx_->config->wifi_enabled) {
    tickWiFi();
  }
}
