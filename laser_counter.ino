/*
  Laser Beam Counter
  Hardware : NodeMCU v3 (ESP8266 / CH340)
             KY-008  – laser emitter   (S pin → 3V3, always-on)
             KY-018  – photoresistor   (S pin → D5 / A0)
             TM1637  – 4-digit display (CLK → D1, DIO → D2)
             Buzzer  – active buzzer   (+ → D7, - → GND)
             Start/Stop btn          (D5 → GND, INPUT_PULLUP)
             LM2596  – buck converter  (set to 5 V)

  Logic    : KY-018 reads LOWER analog value when the laser hits it.
             When an object breaks the beam the value rises above
             THRESHOLD → counter increments, then waits for beam
             to restore before counting again (debounce).

  WiFi     : On first boot, creates AP "LaserCounter-Setup".
             Connect to it and enter your WiFi credentials.
             Once connected, visit the device IP for a live dashboard.

  Libraries needed (install via Arduino Library Manager):
    - TM1637Display  by Avishay Orpaz
    - WiFiManager    by tzapu
*/

#include <TM1637Display.h>
#include "wifi_dashboard.h"

// ── Pin definitions ────────────────────────────────────────────
#define CLK_PIN   D1    // TM1637 clock
#define DIO_PIN   D2    // TM1637 data
#define SENSOR_PIN A0   // KY-018 analog output (NodeMCU A0 = 0-1 V range)

// Optional reset button (connect between D6 and GND, uses INPUT_PULLUP)
#define RESET_BTN D6
#define BUZZER_PIN D7   // active buzzer (+ → D7, - → GND)
#define STARTSTOP_BTN D5  // start/stop toggle (D5 → GND, INPUT_PULLUP)

const int BEEP_MS = 50; // buzzer beep duration on each count

// ── Tunable constants ──────────────────────────────────────────
// Raise THRESHOLD if spurious counts occur in bright environments.
// Lower it if the counter misses objects. Typical ambient-lit room
// with the laser hitting the LDR directly reads ~80-200 (0-1023 scale).
const int THRESHOLD      = 400;   // above this = beam broken
const int DEBOUNCE_MS    = 200;   // ms to wait after a count before re-arming

// ── Globals ───────────────────────────────────────────────────
TM1637Display display(CLK_PIN, DIO_PIN);

int  count        = 0;
bool beamBroken   = false;
bool running      = true;
unsigned long lastCountTime = 0;
int  lastSensorVal = 0;

// Non-blocking buzzer state
bool buzzerActive = false;
unsigned long buzzerStart = 0;

void updateDisplay(int val) {
  display.showNumberDec(val, 0, true);
}

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(RESET_BTN, INPUT_PULLUP);
  pinMode(STARTSTOP_BTN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  display.setBrightness(7);       // 0 (dim) – 7 (brightest)
  display.showNumberDec(0, 0, true);   // show "0000" on boot

  // ── WiFi + web dashboard ─────────────────────────────────────
  wifiSetup(RESET_BTN);
  webServerSetup(count, beamBroken, running, lastSensorVal, updateDisplay);

  Serial.println("Laser counter ready.");
  Serial.println("Cover sensor to calibrate THRESHOLD.");
  addLog("Laser counter ready");
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
    display.showNumberDec(count, 0, true);
    Serial.println("Count reset to 0");
    addLog("Count reset (button)");
    delay(300);   // simple button debounce
  }

  // ── Handle web clients ──────────────────────────────────────
  server.handleClient();

  // ── Read sensor ───────────────────────────────────────────
  int sensorVal = analogRead(SENSOR_PIN);
  lastSensorVal = sensorVal;

  // ── Beam-break detection (only when running) ────────────────
  bool currentlyBroken = (sensorVal > THRESHOLD);

  if (running && currentlyBroken && !beamBroken) {
    // Rising edge: beam just broke
    unsigned long now = millis();
    if (now - lastCountTime > DEBOUNCE_MS) {
      count++;
      lastCountTime = now;

      // Cap at 9999 for 4-digit display
      if (count > 9999) count = 0;

      display.showNumberDec(count, 0, true);
      digitalWrite(BUZZER_PIN, HIGH);
      buzzerActive = true;
      buzzerStart = millis();
      char logMsg[40];
      snprintf(logMsg, sizeof(logMsg), "Count: %d (sensor: %d)", count, sensorVal);
      Serial.println(logMsg);
      addLog(logMsg);
    }
    beamBroken = true;
  }

  if (!currentlyBroken) {
    beamBroken = false;   // beam restored, ready for next object
  }

  // ── Non-blocking buzzer off ──────────────────────────────────
  if (buzzerActive && (millis() - buzzerStart >= BEEP_MS)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }

  // Debug: print raw sensor value every 500 ms (comment out when tuned)
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    Serial.print("Sensor: ");
    Serial.println(sensorVal);
    lastPrint = millis();
  }

  delay(10);
}
