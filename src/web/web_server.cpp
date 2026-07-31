#include "web/web_server.h"

#include <ArduinoJson.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config.h"
#include "control/irrigation_controller.h"
#include "storage/settings_store.h"

extern SettingsStore g_settings;

namespace {
WebServer server(80);

const char kDashboardHtml[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>AutoIrrigation</title>
<style>
:root{--bg:#0f1419;--card:#1a2332;--accent:#00cc88;--warn:#ff5555;--text:#e6edf3;--muted:#8b949e}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,sans-serif;background:var(--bg);color:var(--text);padding:16px}
h1{color:var(--accent);margin-bottom:12px;font-size:1.2rem}
.grid{display:grid;gap:12px;max-width:520px;margin:0 auto}
.card{background:var(--card);border-radius:12px;padding:16px;border:1px solid #30363d}
.row{display:flex;justify-content:space-between;margin:6px 0;font-size:.9rem}
.label{color:var(--muted)}
.zone{font-size:1.5rem;color:var(--accent);font-weight:700}
input,select{width:100%;padding:10px;margin:6px 0;border:1px solid #30363d;border-radius:8px;background:#0d1117;color:var(--text)}
button{padding:12px;border:none;border-radius:8px;font-weight:600;cursor:pointer;margin:4px 0;width:100%}
.btn-go{background:var(--accent);color:#0f1419}
.btn-stop{background:var(--warn);color:#fff}
.btn-save{background:#388bfd;color:#fff}
.msg{font-size:.8rem;color:var(--muted);text-align:center;margin-top:8px}
</style>
</head>
<body>
<div class="grid">
<h1>AutoIrrigation</h1>
<div class="card" id="summary">加载中...</div>
<div class="card" id="zones"></div>
<div class="card">
  <label>手动浇水</label>
  <select id="manualZone"></select>
  <input type="number" id="manualMl" value="100" min="10" max="2000">
  <button class="btn-go" onclick="irrigate()">浇水</button>
  <button class="btn-stop" onclick="estop()">急停</button>
</div>
<div class="card">
  <label>WiFi SSID</label>
  <input id="wifiSsid">
  <label>WiFi 密码</label>
  <input id="wifiPass" type="password">
  <button class="btn-save" onclick="saveWifi()">保存 WiFi</button>
</div>
<p class="msg" id="msg"></p>
</div>
<script>
async function api(path,opt){const r=await fetch(path,opt);return r.json()}
function zoneOpts(zones,sel){sel.innerHTML=zones.map(z=>'<option value="'+z.id+'">'+z.name+'</option>').join('')}
async function refresh(){
  const d=await api('/api/status');
  document.getElementById('summary').innerHTML=
    '<div class="row"><span class="label">状态</span><span>'+d.state+' / '+d.safety+'</span></div>'+
    '<div class="row"><span class="label">水泵</span><span>'+(d.pump?'ON':'OFF')+'</span></div>'+
    '<div class="row"><span class="label">今日流量</span><span>'+d.daily_ml+' ml</span></div>'+
    '<div class="row"><span class="label">WiFi</span><span>'+(d.wifi&&d.wifi.connected?d.wifi.ip:'未连接')+'</span></div>';
  document.getElementById('zones').innerHTML=d.zones.map(z=>
    '<div style="margin-bottom:12px"><div class="zone">'+z.name+': '+z.moisture_pct+'%</div>'+
    '<div class="row"><span class="label">自动</span><span>'+(z.auto_enabled?'开':'关')+'</span></div>'+
    '<div class="row"><span class="label">阈值</span><span>'+z.moisture_low+'% ~ '+z.moisture_high+'%</span></div>'+
    '<div class="row"><span class="label">单次体积</span><span>'+z.volume_ml+' ml</span></div></div>').join('');
  zoneOpts(d.zones,document.getElementById('manualZone'));
}
async function irrigate(){
  const zone=+document.getElementById('manualZone').value;
  const volume_ml=+document.getElementById('manualMl').value;
  await api('/api/irrigate',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({zone,volume_ml})});
  document.getElementById('msg').textContent='已加入队列';
  refresh();
}
async function estop(){await api('/api/emergency-stop',{method:'POST'});refresh()}
async function saveWifi(){
  await api('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({ssid:document.getElementById('wifiSsid').value,password:document.getElementById('wifiPass').value,enabled:true})});
  document.getElementById('msg').textContent='WiFi 已保存，请重启设备';
}
refresh();setInterval(refresh,2000);
</script>
</body>
</html>)rawliteral";
}  // namespace

void WebServerUi::begin(SystemContextEx *ctx) {
  ctx_ = ctx;
  setupRoutes();
}

void WebServerUi::startWiFi() {
  if (ctx_ == nullptr || !ctx_->config->wifi_enabled) {
    if (WIFI_AP_FALLBACK) {
      WiFi.mode(WIFI_AP);
      WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
      Serial.printf("AP: %s\n", WiFi.softAPIP().toString().c_str());
    }
    wifi_started_ = true;
    server.begin();
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ctx_->config->wifi_ssid, ctx_->config->wifi_pass);
  Serial.printf("WiFi connecting to %s...\n", ctx_->config->wifi_ssid);

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(200);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi connected: %s\n", WiFi.localIP().toString().c_str());
  } else if (WIFI_AP_FALLBACK) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    Serial.printf("AP fallback: %s\n", WiFi.softAPIP().toString().c_str());
  }

  wifi_started_ = true;
  server.begin();
}

void WebServerUi::handleRoot() { server.send_P(200, "text/html", kDashboardHtml); }

void WebServerUi::handleStatus() {
  if (ctx_ == nullptr) {
    server.send(500, "application/json", "{}");
    return;
  }

  JsonDocument doc;
  doc["board"] = BOARD_NAME;
  doc["pump"] = ctx_->status->pump_on;
  doc["state"] = ctx_->controller ? ctx_->controller->stateText() : "IDLE";
  doc["safety"] = ctx_->status->safety == SafetyState::Ok ? "OK" : "FAULT";
  doc["daily_ml"] = ctx_->status->daily_ml;
  doc["active_zone"] = ctx_->status->active_zone;
  doc["queue"] = ctx_->status->queue_len;

  JsonObject wifi = doc["wifi"].to<JsonObject>();
  wifi["connected"] = WiFi.status() == WL_CONNECTED;
  wifi["ip"] = WiFi.localIP().toString();
  wifi["rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;

  JsonArray zones = doc["zones"].to<JsonArray>();
  for (uint8_t i = 0; i < ctx_->config->zone_count; ++i) {
    JsonObject z = zones.add<JsonObject>();
    z["id"] = i;
    z["name"] = ctx_->zones[i].name;
    z["moisture_pct"] = ctx_->zone_status[i].moisture_pct;
    z["moisture_adc"] = ctx_->zone_status[i].moisture_adc;
    z["auto_enabled"] = ctx_->zones[i].auto_enabled;
    z["schedule_enabled"] = ctx_->zones[i].schedule_enabled;
    z["moisture_low"] = ctx_->zones[i].moisture_low;
    z["moisture_high"] = ctx_->zones[i].moisture_high;
    z["volume_ml"] = ctx_->zones[i].volume_ml;
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

  SystemContext sc = {ctx_->config, ctx_->zones, ctx_->zone_status, ctx_->status};
  g_settings.save(sc);
  server.send(200, "application/json", "{\"ok\":true}");
}

void WebServerUi::handleIrrigate() {
  if (ctx_ == nullptr || !server.hasArg("plain") || ctx_->controller == nullptr) {
    server.send(400, "application/json", "{\"ok\":false}");
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

void WebServerUi::handleWifiGet() {
  JsonDocument doc;
  doc["connected"] = WiFi.status() == WL_CONNECTED;
  doc["ip"] = WiFi.localIP().toString();
  if (ctx_ != nullptr) {
    doc["enabled"] = ctx_->config->wifi_enabled;
    doc["ssid"] = ctx_->config->wifi_ssid;
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

  JsonDocument doc;
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
  server.send(200, "application/json", "{\"ok\":true}");
}

void WebServerUi::setupRoutes() {
  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  server.on("/api/settings", HTTP_GET, [this]() { handleSettingsGet(); });
  server.on("/api/settings", HTTP_POST, [this]() { handleSettingsPost(); });
  server.on("/api/irrigate", HTTP_POST, [this]() { handleIrrigate(); });
  server.on("/api/emergency-stop", HTTP_POST, [this]() { handleEmergencyStop(); });
  server.on("/api/wifi", HTTP_GET, [this]() { handleWifiGet(); });
  server.on("/api/wifi", HTTP_POST, [this]() { handleWifiPost(); });
}

void WebServerUi::loop() {
  if (!wifi_started_) {
    startWiFi();
  }
  server.handleClient();

  if (ctx_ != nullptr && ctx_->config->wifi_enabled && WiFi.status() != WL_CONNECTED) {
    const uint32_t now = millis();
    if (now - last_wifi_attempt_ms_ >= WIFI_RECONNECT_INTERVAL_MS) {
      last_wifi_attempt_ms_ = now;
      WiFi.reconnect();
    }
  }
}
