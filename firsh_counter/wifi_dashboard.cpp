#include "wifi_dashboard.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

ESP8266WebServer server(80);

// Pointers to main sketch state
static int  *pCount;
static bool *pFishInGate;
static bool *pRunning;
static int  *pLastSensorVal;
static void (*pUpdateDisplay)(int);

// ── Log ring buffer ──────────────────────────────────────────
#define LOG_MAX 50
#define LOG_LINE_MAX 80
static char   logBuf[LOG_MAX][LOG_LINE_MAX];
static int    logHead = 0;   // next write position
static int    logCount = 0;  // total entries (caps at LOG_MAX)

void addLog(const char *msg) {
  unsigned long s = millis() / 1000;
  snprintf(logBuf[logHead], LOG_LINE_MAX, "[%02lu:%02lu:%02lu] %s",
           s / 3600, (s % 3600) / 60, s % 60, msg);
  logHead = (logHead + 1) % LOG_MAX;
  if (logCount < LOG_MAX) logCount++;
}

// ── Web dashboard HTML ────────────────────────────────────────
static const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Fish Counter</title>
<link href="https://fonts.cdnfonts.com/css/digital-7-mono" rel="stylesheet">
<style>
  * { margin:0; padding:0; box-sizing:border-box; }
  body { font-family:-apple-system,sans-serif; background:#0f172a; color:#e2e8f0;
         display:flex; justify-content:center; align-items:center; min-height:100vh; }
  .card { background:#1e293b; border-radius:16px; padding:40px; text-align:center;
          box-shadow:0 8px 32px rgba(0,0,0,.4); min-width:300px; }
  h1 { font-size:1.2rem; color:#94a3b8; margin-bottom:24px; letter-spacing:2px;
       text-transform:uppercase; }
  .count { font-family:'Digital-7 Mono','Digital-7',monospace; font-size:5rem; font-weight:400; color:#38bdf8; letter-spacing:4px; font-variant-numeric:tabular-nums; }
  .sensor { margin-top:20px; font-size:0.9rem; color:#64748b; }
  .bar-bg { margin-top:8px; height:6px; background:#334155; border-radius:3px; overflow:hidden; }
  .bar { height:100%; background:#38bdf8; border-radius:3px; transition:width .3s; }
  .beam { margin-top:16px; font-size:0.85rem; font-weight:600; padding:4px 12px;
          border-radius:999px; display:inline-block; }
  .beam.ok { background:#064e3b; color:#34d399; }
  .beam.broken { background:#7f1d1d; color:#fca5a5; }
  button { margin-top:24px; padding:10px 28px; border:none; border-radius:8px;
           background:#ef4444; color:#fff; font-size:1rem; cursor:pointer;
           transition:background .2s; }
  button:hover { background:#dc2626; }
  .ip { margin-top:16px; font-size:0.75rem; color:#475569; }
  .log-wrap { margin-top:24px; text-align:left; }
  .log-title { font-size:0.8rem; color:#64748b; text-transform:uppercase; letter-spacing:1px;
               margin-bottom:8px; }
  .log { background:#0f172a; border-radius:8px; padding:12px; height:180px;
         overflow-y:auto; font-family:'Courier New',monospace; font-size:0.75rem;
         color:#94a3b8; line-height:1.6; }
</style>
</head>
<body>
<div class="card">
  <h1>Fish Counter</h1>
  <div class="count" id="c">--</div>
  <div class="sensor">Sensor: <span id="s">--</span> / 1023</div>
  <div class="bar-bg"><div class="bar" id="b" style="width:0%"></div></div>
  <div><span class="beam ok" id="st">--</span></div>
  <div style="margin-top:24px">
    <button id="tb" onclick="toggle()" style="background:#22c55e">Start</button>
    <button onclick="reset()">Reset Counter</button>
  </div>
  <div class="log-wrap">
    <div class="log-title">Live Log</div>
    <div class="log" id="log"></div>
  </div>
  <div class="ip" id="ip"></div>
  <div style="margin-top:12px; font-size:0.7rem; color:#334155;">Developed by James Jomuad</div>
</div>
<script>
function poll(){
  fetch("/api/status").then(r=>r.json()).then(d=>{
    document.getElementById("c").textContent=d.count;
    document.getElementById("s").textContent=d.sensor;
    document.getElementById("b").style.width=(d.sensor/1023*100)+"%";
    const st=document.getElementById("st");
    if(d.fish_in_gate){st.textContent="FISH IN GATE";st.className="beam broken";}
    else{st.textContent="GATE CLEAR";st.className="beam ok";}
    const tb=document.getElementById("tb");
    if(d.running){tb.textContent="Stop";tb.style.background="#ef4444";}
    else{tb.textContent="Start";tb.style.background="#22c55e";}
  }).catch(()=>{});
}
function toggle(){
  fetch("/api/toggle",{method:"POST"}).then(()=>poll());
}
function reset(){
  fetch("/api/reset",{method:"POST"}).then(()=>poll());
}
function pollLogs(){
  fetch("/api/logs").then(r=>r.json()).then(d=>{
    const el=document.getElementById("log");
    el.innerHTML=d.logs.map(l=>"<div>"+l+"</div>").join("");
    el.scrollTop=el.scrollHeight;
  }).catch(()=>{});
}
setInterval(poll,500);
setInterval(pollLogs,1000);
poll();
pollLogs();
</script>
</body>
</html>
)rawliteral";

// ── Handlers ──────────────────────────────────────────────────
static void handleRoot() {
  server.send_P(200, "text/html", PAGE_HTML);
}

static void handleStatus() {
  String json = "{\"count\":";
  json += *pCount;
  json += ",\"sensor\":";
  json += *pLastSensorVal;
  json += ",\"fish_in_gate\":";
  json += *pFishInGate ? "true" : "false";
  json += ",\"running\":";
  json += *pRunning ? "true" : "false";
  json += "}";
  server.send(200, "application/json", json);
}

static void handleToggle() {
  *pRunning = !(*pRunning);
  const char *msg = *pRunning ? "Counting started via web" : "Counting stopped via web";
  Serial.println(msg);
  addLog(msg);
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handleReset() {
  *pCount = 0;
  pUpdateDisplay(0);
  Serial.println("Count reset via web");
  addLog("Count reset via web");
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handleLogs() {
  String json = "{\"logs\":[";
  int start = (logCount < LOG_MAX) ? 0 : logHead;
  for (int i = 0; i < logCount; i++) {
    int idx = (start + i) % LOG_MAX;
    if (i > 0) json += ",";
    json += "\"";
    // Escape any quotes in the log line
    for (int c = 0; logBuf[idx][c]; c++) {
      if (logBuf[idx][c] == '"') json += "\\\"";
      else json += logBuf[idx][c];
    }
    json += "\"";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void clearWifiSettings() {
  WiFiManager wm;
  wm.resetSettings();
  Serial.println("WiFi credentials cleared. Restarting...");
  addLog("WiFi credentials cleared via serial — restarting");
  delay(1000);
  ESP.restart();
}

// ── Public functions ──────────────────────────────────────────
void wifiSetup(int resetBtnPin) {
  WiFiManager wm;
  if (digitalRead(resetBtnPin) == LOW) {
    Serial.println("Reset button held – clearing WiFi credentials");
    wm.resetSettings();
  }
  wm.setConnectTimeout(10);        // 10s max to join saved WiFi
  wm.setConfigPortalTimeout(180);
  wm.setCaptivePortalEnable(false); // serve custom dashboard at 192.168.4.1
  if (!wm.autoConnect("FishCounter-Setup")) {
    Serial.println("WiFi not configured – running offline");
  } else {
    Serial.print("Connected! IP: ");
    Serial.println(WiFi.localIP());
  }
}

void webServerSetup(int &count, bool &fishInGate, bool &running, int &lastSensorVal, void (*updateDisplay)(int)) {
  pCount         = &count;
  pFishInGate    = &fishInGate;
  pRunning       = &running;
  pLastSensorVal = &lastSensorVal;
  pUpdateDisplay = updateDisplay;

  server.on("/", handleRoot);
  server.on("/api/status", handleStatus);
  server.on("/api/toggle", HTTP_POST, handleToggle);
  server.on("/api/reset", HTTP_POST, handleReset);
  server.on("/api/logs", handleLogs);
  server.begin();
}
