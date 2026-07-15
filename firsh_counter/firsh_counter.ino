/*
  Fish Counter — Resistivity Gate
  Counts small tilapia fingerlings passing through a 12 mm tube.

  Hardware : NodeMCU v3 (ESP8266 / CH340)
             Conductivity gate  (envelope → A0)
                 - 2x 316 stainless ring electrodes in the bore
                 - TLC555 ~2 kHz AC drive, BAT43 envelope detector,
                   MCP6002 buffer (see diagrams/resistivity_sensor.md)
             TM1637  – 4-digit display (CLK → D1, DIO → D2)
             Buzzer  – active buzzer   (+ → D7, - → GND)
             Start/Stop btn           (D5 → GND, INPUT_PULLUP)
             LM2596  – buck converter  (set to 5 V)

  Logic    : A0 reads the rectified conductivity level across the
             electrode gap. The code learns the clear-water baseline
             continuously; when a fish enters the gap the reading
             deviates from baseline by more than DETECT_DELTA (either
             direction) → counter increments, then waits for the gap
             to read clear again (re-arm + debounce) before counting.

  WiFi     : On first boot, creates AP "LaserCounter-Setup".
             Connect to it and enter your WiFi credentials.
             Once connected, visit the device IP for a live dashboard.

  Libraries needed (install via Arduino Library Manager):
    - TM1637Display  by Avishay Orpaz
    - WiFiManager    by tzapu
*/

#include <TM1637Display.h>
#include <ESP8266WiFi.h>
#include "wifi_dashboard.h"
#include "wifi_setup.h"

// ── Pin definitions ────────────────────────────────────────────
#define CLK_PIN   D1    // TM1637 clock
#define DIO_PIN   D2    // TM1637 data
#define SENSOR_PIN A0   // conductivity envelope from the electrode gate

// Optional reset button (connect between D6 and GND, uses INPUT_PULLUP)
#define RESET_BTN D6
#define BUZZER_PIN D7   // active buzzer (+ → D7, - → GND)
#define STARTSTOP_BTN D5  // start/stop toggle (D5 → GND, INPUT_PULLUP)

const int BEEP_MS = 50; // buzzer beep duration on each count

// ── Tunable constants ──────────────────────────────────────────
// Detection uses a self-calibrating baseline instead of a fixed
// threshold: the code learns the clear-water reading continuously,
// then counts when the live reading deviates from it by DETECT_DELTA
// (in EITHER direction — a fish may raise or lower conductivity).
// This cancels out drift from water temperature / salinity.
// Tuning: with no fish, set the Rs trimpot so A0 sits mid-scale, then
// watch the serial "Dev" value for an empty gap vs. a fish in the gap
// and set DETECT_DELTA between them. Lower it to catch fainter fish;
// raise it if water ripple / bubbles cause false counts.
const int DETECT_DELTA   = 300;   // min deviation from baseline to count
const int DEBOUNCE_MS    = 200;   // ms to wait after a count before re-arming
const int REARM_MS       = 80;    // gap must read clear this long before re-arming
const float BASE_ALPHA   = 0.02;  // baseline adaptation rate (0-1, higher = faster)
const float SENSOR_ALPHA = 0.4;  // sensor noise filter (0-1, lower = smoother)

// ── Globals ───────────────────────────────────────────────────
TM1637Display display(CLK_PIN, DIO_PIN);

int  count        = 0;
bool fishInGate   = false;          // true while a fish is in the gate
bool running      = true;
unsigned long lastCountTime = 0;
unsigned long clearSince    = 0;    // when the gap last read clear (for re-arm)
int  lastSensorVal = 0;
float baseline     = 0;             // self-calibrating clear-water reading
unsigned long brokenSince = 0;      // anti-stuck timer: when "broken" state began

// Non-blocking buzzer state
bool buzzerActive = false;
unsigned long buzzerStart = 0;

// EMA filter state for sensor noise reduction
float filteredSensor = 0;
bool filteredSensorInit = false;

void updateDisplay(int val) {
  display.showNumberDec(val, true, 4);
}

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(RESET_BTN, INPUT_PULLUP);
  pinMode(STARTSTOP_BTN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // ── Force TM1637 bus into known state ──────────────────────
  pinMode(CLK_PIN, OUTPUT);
  pinMode(DIO_PIN, OUTPUT);
  digitalWrite(CLK_PIN, HIGH);
  digitalWrite(DIO_PIN, HIGH);
  delay(1);
  for (int i = 0; i < 10; i++) {
    digitalWrite(CLK_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(CLK_PIN, HIGH);
    delayMicroseconds(5);
  }
  digitalWrite(DIO_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(CLK_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(CLK_PIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(DIO_PIN, HIGH);
  delayMicroseconds(5);

  display.setBrightness(7);       // 0 (dim) – 7 (brightest)

  // ── Boot countdown ──────────────────────────────────────────
  for (int i = 8; i >= 0; i--) {
    display.showNumberDec(i * 1111, false, 4);
    delay(400);
  }

  // Seed the baseline with the current clear-water reading
  baseline = analogRead(SENSOR_PIN);

  // ── WiFi + web dashboard ─────────────────────────────────────
  wifiSetup(RESET_BTN);
  webServerSetup(count, fishInGate, running, lastSensorVal, updateDisplay);

  // ── Show IP on display ───────────────────────────────────────
  if (WiFi.isConnected()) {
    IPAddress ip = WiFi.localIP();
    for (int cycle = 0; cycle < 2; cycle++) {
      for (int octet = 0; octet < 4; octet++) {
        if (octet < 3) {
          display.showNumberDecEx(ip[octet], 0x01, false, 4); // dot on rightmost digit
        } else {
          display.showNumberDec(ip[octet], false, 4);
        }
        delay(700);
      }
    }
  }
  display.showNumberDec(count, true, 4);

  Serial.println("Fish counter ready.");
  Serial.println("Pass a fish through the gate to tune DETECT_DELTA.");
  addLog("Fish counter ready");
}

// ── Main loop ─────────────────────────────────────────────────
void loop() {

  // ── Start/Stop button ────────────────────────────────────
  if (digitalRead(STARTSTOP_BTN) == LOW) {
    running = !running;
    Serial.println(running ? "Counting started" : "Counting stopped");
    addLog(running ? "Counting started (button)" : "Counting stopped (button)");
    delay(300);   // simple button debounce
  }

  // ── Reset button ──────────────────────────────────────────
    if (digitalRead(RESET_BTN) == LOW) {
    count = 0;
    display.showNumberDec(0, true, 4);
    Serial.println("Count reset to 0");
    addLog("Count reset (button)");
    delay(300);   // simple button debounce
  }

  // ── Serial commands ────────────────────────────────────────
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.equalsIgnoreCase("RESETWIFI") || cmd.equalsIgnoreCase("CLEARWIFI")) {
      clearWifiSettings();
    } else if (cmd.startsWith("ADD")) {
      int n = cmd.substring(3).toInt();
      if (n < 1) n = 1;
      count += n;
      if (count > 9999) count = 0;
      display.showNumberDec(count, true, 4);
      char logMsg[32];
      snprintf(logMsg, sizeof(logMsg), "Add %d -> count %d", n, count);
      Serial.println(logMsg);
      addLog(logMsg);
    }
  }

  // ── Handle web clients ──────────────────────────────────────
  handleSSEClients();

  // ── Read sensor with EMA noise filter ─────────────────────
  int rawVal = analogRead(SENSOR_PIN);
  if (!filteredSensorInit) {
    filteredSensor = rawVal;
    filteredSensorInit = true;
  }
  filteredSensor += (rawVal - filteredSensor) * SENSOR_ALPHA;
  int sensorVal = (int)filteredSensor;
  lastSensorVal = sensorVal;
  unsigned long now = millis();

  // ── Fish detection: deviation from self-calibrating baseline ─
  // A fish in the gate shifts conductivity away from the learned
  // clear-water baseline in either direction.
  int deviation = abs(sensorVal - (int)baseline);
  bool currentlyBroken = (deviation > DETECT_DELTA);

  if (running && currentlyBroken && !fishInGate) {
    // Rising edge: a fish just entered the gate
    if (now - lastCountTime > DEBOUNCE_MS) {
      count++;
      lastCountTime = now;

      // Cap at 9999 for 4-digit display
      if (count > 9999) count = 0;

      display.showNumberDec(count, true, 4);
      digitalWrite(BUZZER_PIN, HIGH);
      buzzerActive = true;
      buzzerStart = millis();
      char logMsg[48];
      snprintf(logMsg, sizeof(logMsg), "Fish #%d (dev: %d)", count, deviation);
      Serial.println(logMsg);
      addLog(logMsg);
    }
    fishInGate = true;
  }

  if (currentlyBroken) {
    clearSince = 0;                       // fish present: not clear
    // ── Anti-stuck: if sensor has been consistently "broken" for
  //     ≥ 5 s the baseline is probably wrong (no fish is that long).
  //     Slowly drift it so the system can recover on its own.
    if (brokenSince == 0) brokenSince = now;
    if (now - brokenSince > 5000) {
      baseline += (sensorVal - baseline) * 0.001;  // very slow creep
    }
  } else {
    brokenSince = 0;  // reset anti-stuck timer — gap is clear
    // Gap clear: adapt the baseline toward the clear-water reading and
    // re-arm only after it has stayed clear for REARM_MS (so one fish's
    // head/body/tail isn't counted as several).
    baseline += (sensorVal - baseline) * BASE_ALPHA;
    if (clearSince == 0) clearSince = now;
    if (now - clearSince > REARM_MS) fishInGate = false;
  }

  // ── Non-blocking buzzer off ──────────────────────────────────
  if (buzzerActive && (millis() - buzzerStart >= BEEP_MS)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }

  // Debug: print sensor, baseline and deviation every 500 ms.
  // Use this to pick DETECT_DELTA: compare Dev with an empty gap vs.
  // a fish in the gate, then set DETECT_DELTA between them.
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    Serial.print("Sensor: ");
    Serial.print(sensorVal);
    Serial.print("  Baseline: ");
    Serial.print((int)baseline);
    Serial.print("  Dev: ");
    Serial.println(deviation);
    lastPrint = millis();
  }

  delay(5);
}
