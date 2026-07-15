# IR Beam-Break Sensor (TSOP38238) — Alternative Counting Method

Alternative to the conductivity gate for counting small tilapia fingerlings
through a 12 mm transparent tube. Uses a **modulated 38 kHz IR beam** and a
**TSOP38238 IR receiver** — the standard demodulating IR receiver found in
remote-control applications.

## When to use this over the conductivity gate

| Situation | Recommendation |
|---|---|
| Tube water is very clean / low salinity | IR beam may work | 
| Water is turbid or has suspended solids | Conductivity gate more reliable |
| Fish are very small (<3 cm) | Conductivity gate more sensitive |
| Simple proof-of-concept / no TLC555 available | IR beam is simpler |
| Final production deployment | Conductivity gate recommended |

## How it works

1. A **TLC555 astable driver** generates a 38 kHz square wave (matching the
   TSOP38238's bandpass filter).
2. An **IR LED** (940 nm) driven by that 38 kHz signal shines across the tube.
3. The **TSOP38238** on the opposite side demodulates the signal: its output
   goes **LOW** while the 38 kHz beam is intact.
4. A fish passing through the tube breaks the beam — the TSOP output goes
   **HIGH** (internal pull-up).
5. The ESP8266 reads the TSOP output on a digital GPIO pin.

> The TSOP38238 includes an internal bandpass filter and automatic gain
> control — it ignores ambient IR (sunlight, room lights) and only responds
> to the modulated 38 kHz signal.

## TSOP38238 Pinout

Viewed from the front (bulge/IR window facing you):

```
     ┌──────┐
     │ ═══  │  ← IR window
     │      │
     └──┬───┘
        │
   ┌────┴────┐
   │         │
  OUT GND  VCC
  (1)  (2)  (3)
```

| Pin | Name | Notes |
|---|---|---|
| 1 | OUT | Open-collector, active LOW when 38 kHz carrier detected |
| 2 | GND | Ground |
| 3 | VCC | 2.5–5.5 V (connect to NodeMCU 3V3 or 5V) |

**Output logic:**
- Beam intact (38 kHz received): OUT = LOW
- Beam broken (no 38 kHz): OUT = HIGH (internal 30 kΩ pull-up to VCC)

## IR LED (Emitter)

| Part | Typical Spec |
|---|---|
| 5 mm IR LED, 940 nm | Vf ~1.3–1.5 V, If ~20–100 mA |
| Current-limiting resistor | 100–220 Ω at 3V3 (handle 50–100 mA peak) |

## Wiring

### TLC555 (38 kHz Astable)

| Pin | Name | Connects To | Notes |
|---|---|---|---|
| 1 | GND | GND rail | |
| 2 | TRIG | TLC555 pin 6 (THRESH) | Tied for astable |
| 3 | OUT | IR LED anode (via R_lim) | 38 kHz drive signal |
| 4 | RESET | TLC555 pin 8 (VCC) | Pulled high |
| 5 | CTRL | 10 nF → GND | Bypass |
| 6 | THRESH | pin 2 + R2 → Ct | Timing junction |
| 7 | DISCH | R1 → VCC, R2 → Ct | Discharge |
| 8 | VCC | NodeMCU 3V3 | Supply |

**Timing components for ~38 kHz:**
- R1 = 1 kΩ, R2 = 10 kΩ, Ct = 2.2 nF → ~38 kHz
- Adjust R2 or Ct slightly to centre the TSOP's bandpass.

### IR LED

| From | To | Notes |
|---|---|---|
| TLC555 pin 3 (OUT) | R_lim (100–220 Ω) | Current limit |
| R_lim | IR LED anode | |
| IR LED cathode | GND | |

### TSOP38238 (Receiver)

| TSOP Pin | Connects To | Notes |
|---|---|---|
| 1 (OUT) | NodeMCU D8 (GPIO15) or any free digital GPIO | Digital read; LOW = beam intact |
| 2 (GND) | GND rail | |
| 3 (VCC) | NodeMCU 3V3 | |

> **NodeMCU D8 (GPIO15)** is a good choice because it defaults to HIGH on boot
> and doesn't affect the flash boot sequence. Alternatively use D0–D7 if D8 is
> needed elsewhere — just avoid GPIO0, GPIO2, and GPIO16 for boot safety.

### Decoupling Cap (Critical)

| From | To | Value |
|---|---|---|
| TSOP38238 pin 3 (VCC) | GND | **10–100 µF electrolytic + 100 nF ceramic** as close to the module as possible |

The TSOP draws brief current spikes when demodulating; the cap prevents
voltage droop and false triggers.

## ASCII Wiring Overview

```
                     NodeMCU v3
                   +--------------+
                   |              |
         TSOP OUT ─│ D8 (GPIO15)  │
                   |  3V3 ────────│──→ TLC555 VCC, TSOP VCC
                   |  GND ────────│──→ GND rail
                   +--------------+

   TLC555 (38 kHz astable)         IR LED        12 mm tube       TSOP38238
   ┌──────────────────────┐      ┌──────┐      ┌──────────┐      ┌────────┐
   │                 OUT ─┼──┬──→│Anode │      │          │      │ OUT ───→ D8
   │                 VCC ─┼──┤   │      │      │    ──    │      │ VCC ───→ 3V3
   │                 GND ─┼──┤   │Cath.─┼──GND │  ── ──   │      │ GND ───→ GND
   └──────────────────────┘  │   └──────┘      │  ── ──   │      └────────┘
                             │                │    ──    │
                             R_lim             └──────────┘
                             100-220 Ω          ◄── beam ──►
```

## Bill of Materials

| Part | Value / Type | Role |
|---|---|---|
| TSOP38238 | 38 kHz IR receiver module | Demodulated IR detector |
| TLC555 (CMOS 555) | timing: R1 1 k, R2 10 k, Ct 2.2 nF | 38 kHz astable driver |
| IR LED | 5 mm 940 nm, 20–100 mA | IR emitter |
| R_lim | 100–220 Ω (1/4 W) | IR LED current limiting |
| Ct | 2.2 nF ceramic | TLC555 timing cap |
| R1, R2 | 1 kΩ, 10 kΩ (1/4 W) | TLC555 timing resistors |
| C_bypass | 100 nF ceramic | TLC555 pin 5 bypass |
| C_decouple | 10–100 µF electrolytic + 100 nF | TSOP VCC decoupling |

## Mounting

- **IR LED** and **TSOP38238** face each other across the 12 mm tube, on
  opposite sides, with a **≤10 mm gap** between each component and the tube
  wall (total beam path ~32 mm).
- Black heat-shrink tubing over the TSOP window can reduce stray ambient IR
  from hitting the receiver.
- Align the LED and receiver so the beam passes cleanly through the centre of
  the tube bore.

## Firmware Changes

The existing `laser_counter.ino` detection logic (deviation from baseline) is
designed for the conductivity gate. For the IR beam, replace the main loop's
sensor section with:

```cpp
#define IR_PIN D8   // TSOP38238 output

// In loop():
bool beamBroken = (digitalRead(IR_PIN) == HIGH);   // HIGH = no carrier

if (running && beamBroken && !fishInGate) {
  if (now - lastCountTime > DEBOUNCE_MS) {
    count++;
    // ... same display, buzzer, log as existing code
  }
  fishInGate = true;
}

if (!beamBroken) {
  if (clearSince == 0) clearSince = now;
  if (now - clearSince > REARM_MS) fishInGate = false;
} else {
  clearSince = 0;
}
```

No baseline learning is needed — the TSOP output is a clean digital HIGH/LOW.
`DETECT_DELTA`, `BASE_ALPHA`, and `SENSOR_ALPHA` are unused in this mode.

## Tuning

1. Point the IR LED at the TSOP38238 through the empty tube (filled with
   water). Verify TSOP OUT reads LOW.
2. Break the beam with your finger — verify OUT reads HIGH.
3. Adjust the TLC555 timing (R2 or Ct) if the TSOP doesn't lock: the
   bandpass is narrow (~±2 kHz at 38 kHz).
4. If the TSOP is unreliable, reduce the gap or add a lens/reflector to
   concentrate IR on the receiver.
