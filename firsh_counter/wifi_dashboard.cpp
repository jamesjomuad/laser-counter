#include "wifi_dashboard.h"
#include <ESP8266WiFi.h>

AsyncWebServer server(80);
AsyncEventSource events("/api/events");

// Pointers to main sketch state
static int  *pCount;
static bool *pFishInGate;
static bool *pRunning;
static int  *pLastSensorVal;
static bool *pIrDetected;
static void (*pUpdateDisplay)(int);

// ── Log ring buffer ──────────────────────────────────────────
#define LOG_MAX 50
#define LOG_LINE_MAX 80
static char   logBuf[LOG_MAX][LOG_LINE_MAX];
static int    logHead = 0;
static int    logCount = 0;

void addLog(const char *msg) {
  unsigned long s = millis() / 1000;
  snprintf(logBuf[logHead], LOG_LINE_MAX, "[%02lu:%02lu:%02lu] %s",
           s / 3600, (s % 3600) / 60, s % 60, msg);
  logHead = (logHead + 1) % LOG_MAX;
  if (logCount < LOG_MAX) logCount++;
}

// ── JSON helpers ─────────────────────────────────────────────
static void buildStatusJSON(String &buf) {
  buf = "{\"count\":";
  buf += *pCount;
  buf += ",\"sensor\":";
  buf += *pLastSensorVal;
  buf += ",\"fish_in_gate\":";
  buf += *pFishInGate ? "true" : "false";
  buf += ",\"running\":";
  buf += *pRunning ? "true" : "false";
  buf += ",\"ir_detected\":";
  buf += *pIrDetected ? "true" : "false";
  buf += "}";
}

static void buildLogsJSON(String &buf) {
  buf = "[";
  int start = (logCount < LOG_MAX) ? 0 : logHead;
  for (int i = 0; i < logCount; i++) {
    int idx = (start + i) % LOG_MAX;
    if (i > 0) buf += ",";
    buf += "\"";
    for (int c = 0; logBuf[idx][c]; c++) {
      if (logBuf[idx][c] == '"') buf += "\\\"";
      else buf += logBuf[idx][c];
    }
    buf += "\"";
  }
  buf += "]";
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
  .status { margin-top:16px; font-size:0.85rem; font-weight:600; padding:4px 12px;
            border-radius:999px; display:inline-block; }
  .status.ok { background:#064e3b; color:#34d399; }
  .status.broken { background:#7f1d1d; color:#fca5a5; }
  .ir-label { margin-top:8px; font-size:0.75rem; color:#64748b; }
  button { margin-top:24px; padding:10px 28px; border:none; border-radius:8px;
           background:#ef4444; color:#fff; font-size:1rem; cursor:pointer;
           transition:background .2s; }
  button:hover { background:#dc2626; }
  .ip { margin-top:16px; font-size:0.75rem; color:#475569; }
  .log-wrap { margin-top:24px; text-align:left; }
  .log-title { font-size:0.8rem; color:#64748b; text-transform:uppercase; letter-spacing:1px;
               margin-bottom:8px; }
  .log { background:#0f172a; border-radius:8px; padding:12px; height:360px;
         overflow-y:auto; font-family:'Courier New',monospace; font-size:0.75rem;
         color:#94a3b8; line-height:1.6; }
</style>
</head>
<body>
<div class="card">
  <h1>Fish Counter</h1>
  <div class="count" id="c">--</div>
  <div><span class="status ok" id="st">--</span></div>
  <div class="ir-label">IR sensor: <span id="s">--</span></div>
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
const evtSource = new EventSource("/api/events");

evtSource.addEventListener("status", function(e){
  const d = JSON.parse(e.data);
  document.getElementById("c").textContent = d.count;
  const st = document.getElementById("st");
  if (d.fish_in_gate) { st.textContent = "COUNTING"; st.className = "status broken"; }
  else { st.textContent = "AWAITING"; st.className = "status ok"; }
  document.getElementById("s").textContent = d.ir_detected ? "OBSTACLE" : "CLEAR";
  const tb = document.getElementById("tb");
  if (d.running) { tb.textContent = "Stop"; tb.style.background = "#ef4444"; }
  else { tb.textContent = "Start"; tb.style.background = "#22c55e"; }
});

evtSource.addEventListener("log", function(e){
  const logs = JSON.parse(e.data);
  const el = document.getElementById("log");
  el.innerHTML = logs.map(l => "<div>" + l + "</div>").join("");
  el.scrollTop = el.scrollHeight;
});

evtSource.addEventListener("error", function(){
  document.getElementById("st").textContent = "RECONNECTING...";
  document.getElementById("st").className = "status broken";
});

function toggle(){
  fetch("/api/toggle", {method:"POST"});
}

function reset(){
  fetch("/api/reset", {method:"POST"});
  document.getElementById("log").innerHTML = "";
}
</script>
</body>
</html>
)rawliteral";

// ── Handlers (AsyncWebServer) ─────────────────────────────────
static void handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", PAGE_HTML);
}

static void handleStatus(AsyncWebServerRequest *request) {
  String json;
  buildStatusJSON(json);
  request->send(200, "application/json", json);
}

static void handleToggle(AsyncWebServerRequest *request) {
  *pRunning = !(*pRunning);
  pUpdateDisplay(*pCount);
  const char *msg = *pRunning ? "Counting started via web" : "Counting stopped via web";
  Serial.println(msg);
  addLog(msg);
  request->send(200, "application/json", "{\"ok\":true}");
}

static void handleReset(AsyncWebServerRequest *request) {
  *pCount = 0;
  logHead = 0;
  logCount = 0;
  pUpdateDisplay(0);
  Serial.println("Count reset via web");
  addLog("Count reset via web");
  request->send(200, "application/json", "{\"ok\":true}");
}

static void handleLogs(AsyncWebServerRequest *request) {
  String json = "{\"logs\":[";
  int start = (logCount < LOG_MAX) ? 0 : logHead;
  for (int i = 0; i < logCount; i++) {
    int idx = (start + i) % LOG_MAX;
    if (i > 0) json += ",";
    json += "\"";
    for (int c = 0; logBuf[idx][c]; c++) {
      if (logBuf[idx][c] == '"') json += "\\\"";
      else json += logBuf[idx][c];
    }
    json += "\"";
  }
  json += "]}";
  request->send(200, "application/json", json);
}

// ── SSE pushes (called from main loop) ───────────────────────
void handleSSEClients() {
  static unsigned long lastStatus = 0;
  static unsigned long lastLog = 0;
  unsigned long now = millis();

  if (now - lastStatus >= 200) {
    lastStatus = now;
    *pLastSensorVal = analogRead(A0);
    String data;
    buildStatusJSON(data);
    events.send(data.c_str(), "status", now);
  }

  if (now - lastLog >= 1500) {
    lastLog = now;
    String data;
    buildLogsJSON(data);
    events.send(data.c_str(), "log", now);
  }
}

// ── Web server setup (WiFi setup is in wifi_setup.cpp) ────────
void webServerSetup(int &count, bool &fishInGate, bool &running, int &lastSensorVal, bool &irDetected, void (*updateDisplay)(int)) {
  pCount         = &count;
  pFishInGate    = &fishInGate;
  pRunning       = &running;
  pLastSensorVal = &lastSensorVal;
  pIrDetected    = &irDetected;
  pUpdateDisplay = updateDisplay;

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/toggle", HTTP_POST, handleToggle);
  server.on("/api/reset", HTTP_POST, handleReset);
  server.on("/api/logs", HTTP_GET, handleLogs);

  events.onConnect([](AsyncEventSourceClient *client) {
    client->send("connected", NULL, millis(), 2000);
  });
  server.addHandler(&events);

  server.begin();
}
