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

// Optional reset button (connect between D6 and GND, uses INPUT_PULLUP)
#define RESET_BTN D6
#define BUZZER_PIN D7   // active buzzer (+ → D7, - → GND)
#define STARTSTOP_BTN D5  // start/stop toggle (D5 → GND, INPUT_PULLUP)
#define IR_PIN D4         // reflective IR sensor: LOW = obstacle detected

// ── Tunable constants ──────────────────────────────────────────
const int DEBOUNCE_MS    = 200;   // ms to wait after a count before re-arming
const int REARM_MS       = 80;    // gap must read clear this long before re-arming
const int BEEP_MS        = 50;    // buzzer beep duration per count

// ── Globals ───────────────────────────────────────────────────
TM1637Display display(CLK_PIN, DIO_PIN);

int  count        = 0;
bool fishInGate   = false;          // true while a fish is in the gate
bool running      = true;
bool irDetected   = false;          // reflective IR sensor state
static bool irPrev = false;
unsigned long lastCountTime = 0;
unsigned long clearSince    = 0;    // when the gap last read clear (for re-arm)
int  lastSensorVal = 0;
unsigned long brokenSince = 0;      // anti-stuck timer: when "broken" state began

// Non-blocking buzzer state
bool buzzerActive = false;
unsigned long buzzerStart = 0;

void showStop() {
  Serial.println("showStop() called");
  const uint8_t seg[] = {0x6D, 0x78, 0x3F, 0x73}; // S t O P
  display.setSegments(seg, 4, 0);
}

void updateDisplay(int val) {
  if (running) {
    display.showNumberDec(val, true, 4);
  } else {
    showStop();
  }
}

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(RESET_BTN, INPUT_PULLUP);
  pinMode(STARTSTOP_BTN, INPUT_PULLUP);
  pinMode(IR_PIN, INPUT_PULLUP);
  irDetected = (digitalRead(IR_PIN) == LOW);
  irPrev = irDetected;
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  display.setBrightness(7);       // 0 (dim) – 7 (brightest)

  // ── Boot countdown ──────────────────────────────────────────
  for (int i = 8; i >= 0; i--) {
    display.showNumberDec(i * 1111, false, 4);
    delay(400);
  }

  // ── WiFi + web dashboard ─────────────────────────────────────
  wifiSetup(RESET_BTN);
  webServerSetup(count, fishInGate, running, lastSensorVal, irDetected, updateDisplay);

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

  Serial.println("Fish counter ready — IR sensor mode.");
  addLog("Fish counter ready");
}

// ── Main loop ─────────────────────────────────────────────────
void loop() {

  // ── Start/Stop button ────────────────────────────────────
  if (digitalRead(STARTSTOP_BTN) == LOW) {
    running = !running;
    if (running) {
      display.showNumberDec(count, true, 4);
    } else {
      showStop();
    }
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

  unsigned long now = millis();

  // ── Read reflective IR sensor ───────────────────────────────
  bool irNow = (digitalRead(IR_PIN) == LOW);
  bool irRising = irNow && !irPrev;   // just became LOW (obstacle detected)
  irPrev = irNow;
  irDetected = irNow;

  // ── Fish detection ─────────────────────────────────────────
  bool currentlyBroken = irNow;

  if (running && irRising && !fishInGate) {
    if (now - lastCountTime > DEBOUNCE_MS) {
      count++;
      lastCountTime = now;
      if (count > 9999) count = 0;

      display.showNumberDec(count, true, 4);
      digitalWrite(BUZZER_PIN, HIGH);
      buzzerActive = true;
      buzzerStart = millis();
      char logMsg[48];
      snprintf(logMsg, sizeof(logMsg), "Fish #%d (IR)", count);
      Serial.println(logMsg);
      addLog(logMsg);
    }
    fishInGate = true;
  }

  if (currentlyBroken) {
    clearSince = 0;
    // Anti-stuck: if blocked ≥ 5 s, assume false positive and clear
    if (brokenSince == 0) brokenSince = now;
    if (now - brokenSince > 5000) {
      fishInGate = false;
      brokenSince = now;
      Serial.println("IR stuck timeout — cleared");
    }
  } else {
    brokenSince = 0;
    if (clearSince == 0) clearSince = now;
    if (now - clearSince > REARM_MS) fishInGate = false;
  }

  // ── Non-blocking buzzer off ──────────────────────────────────
  if (buzzerActive && (millis() - buzzerStart >= BEEP_MS)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }

  // Debug: print state every 500 ms.
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    Serial.print("Count: ");
    Serial.print(count);
    Serial.print("  IR: ");
    Serial.print(digitalRead(IR_PIN));
    Serial.print("  fishInGate: ");
    Serial.println(fishInGate ? "YES" : "no");
    lastPrint = millis();
  }

  delay(5);
}
