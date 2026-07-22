# Fish Counter — Wiring Table & Pin Map

Sensor is a **conductivity (resistivity) gate**, not a laser beam. Full
electrode + analog front-end design is in `resistivity_sensor.md`; this file
documents every wire between every component.

---

## NodeMCU v3 (ESP8266)

| Pin | GPIO | Connects To | Notes |
|---|---|---|---|
| A0 | ADC0 | MCP6002 pin 1 (OUT) | Only analog input; rectified gate envelope |
| D1 | GPIO5 | TM1637 CLK | |
| D2 | GPIO4 | TM1637 DIO | |
| D5 | GPIO14 | Start/Stop button terminal 1 | INPUT_PULLUP; press connects to GND |
| D6 | GPIO12 | Reset button terminal 1 | INPUT_PULLUP; press connects to GND |
| D7 | GPIO13 | Buzzer (+) | HIGH = beep |
| D8 | GPIO15 | Reflective IR sensor OUT | LOW = obstacle detected (INPUT) |
| 3V3 (left) | — | TLC555 pin 8 (VCC), MCP6002 pin 8 (VCC) | Powers sensor front-end |
| GND (left) | — | GND rail (shared) | Common ground |
| VIN | — | LM2596 5V OUT | Board power input |

---

## TLC555 (Astable ~2 kHz)

| Pin | Name | Connects To | Notes |
|---|---|---|---|
| 1 | GND | GND rail | |
| 2 | TRIG | TLC555 pin 6 (THRESH) | Tied together for astable |
| 3 | OUT | Cb (1 µF) → Electrode A | AC square wave to electrode |
| 4 | RESET | TLC555 pin 8 (VCC) | Pulled high to enable |
| 5 | CTRL | 10 nF → GND | Bypass cap to filter noise |
| 6 | THRESH | TLC555 pin 2 (TRIG) + R2 → Cb/Ct | Timing junction |
| 7 | DISCH | R1 → VCC, R2 → timing cap | Discharge pin |
| 8 | VCC | NodeMCU 3V3 | Supply |

**Timing:** R1 = 1 kΩ, R2 = 33 kΩ, Ct = 10 nF → ~2 kHz

---

## Electrodes & DC-Block Caps

| From | To | Notes |
|---|---|---|
| TLC555 pin 3 (OUT) | Cb (1 µF) leg 1 | |
| Cb (1 µF) leg 2 | Electrode A | AC drive to first ring |
| Electrode B | Cb (1 µF) leg 1 | |
| Cb (1 µF) leg 2 | GND rail | DC-block return — no electrolysis |

---

## SENSE Node & Divider

| From | To | Notes |
|---|---|---|
| Electrode A / Cb junction | Rs (10 k trimpot) leg 1 | AC divider input |
| Rs wiper | **SENSE** node | Tap to envelope detector |
| Rs leg 2 | GND | Completes divider |

---

## Envelope Detector

| From | To | Notes |
|---|---|---|
| SENSE node | BAT43 anode | Half-wave rectify |
| BAT43 cathode | Ce (1 µF) + Rb (10 kΩ) |||
| Ce / Rb junction | NodeMCU A0 (via 100 nF cap) | Smoothed DC envelope → ADC |
| Ce (other leg) | GND | |
| Rb (other leg) | GND | |

---

## A0 Noise Filter

| From | To | Value |
|---|---|---|
| NodeMCU A0 | NodeMCU GND (adjacent pin) | **100 nF ceramic** — solder lead as short as possible |

---

## MCP6002 (Buffer — optional, recommended)

*Only needed if you have one. Omit if the Ce/Rb values are 1 µF / 10 kΩ as documented above.*

| Pin | Name | Connects To | Notes |
|---|---|---|---|
| 1 | OUT | NodeMCU A0 | Buffered envelope level |
| 2 | IN− | MCP6002 pin 1 (OUT) | Unity-gain buffer (tied to output) |
| 3 | IN+ | Envelope detector (Ce / Rb junction) | DC envelope input |
| 4 | V− | GND | |
| 8 | V+ | NodeMCU 3V3 | Supply |

*Using the MCP6002? Restore Ce to 100 nF and Rb to 100 kΩ, then insert its output to A0.*

---

## TM1637 Display

| Display Pin | Connects To |
|---|---|
| CLK | NodeMCU D1 (GPIO5) |
| DIO | NodeMCU D2 (GPIO4) |
| VCC | NodeMCU 3V3 (or VIN 5V — module has regulator) |
| GND | GND rail |

---

## Buttons

| Button | Terminal 1 | Terminal 2 |
|---|---|---|
| Start/Stop | NodeMCU D5 (GPIO14) | GND |
| Reset | NodeMCU D6 (GPIO12) | GND |

Both set `INPUT_PULLUP` in firmware → press reads LOW.

---

## Active Buzzer

| Buzzer Pin | Connects To |
|---|---|
| (+) | NodeMCU D7 (GPIO13) |
| (−) | GND |

---

## Power Supply

| From | To | Notes |
|---|---|---|
| LM2596 IN+ | 7–12 V DC barrel jack | Input |
| LM2596 IN− | GND | |
| LM2596 OUT+ | NodeMCU VIN | Set to **5.0 V** |
| LM2596 OUT− | GND rail | |

TLC555 and MCP6002 run from NodeMCU 3V3 so A0 never exceeds 3.3 V.

---

## ASCII Wiring

```
                        NodeMCU v3
                     +--------------+
                     |              |
   MCP6002 OUT ─────→| A0           |
                     |              |
                     |   D1 (GPIO5) |←────── TM1637 CLK
                     |   D2 (GPIO4) |←────── TM1637 DIO
                     |              |
                     |   D5 (GPIO14)|←────── Start/Stop ───→ GND
                     |   D6 (GPIO12)|←────── Reset ─────────→ GND
                      |   D7 (GPIO13)|←────── Buzzer (+)
                      |   D8 (GPIO15)|←────── IR sensor OUT (LOW = object)
                      |              |
                     |          3V3 |──→ TLC555 VCC, MCP6002 V+
                     |          GND |──→ Buzzer (−), buttons, GND rail
                     |          VIN |←── LM2596 5V OUT
                     +--------------+

  TLC555 ─┤Cb├─ Rs ─┬─ Electrode A )(  gap  )( Electrode B ─┤Cb├─ GND
                     └─ SENSE ─► BAT43 ─► Ce//Rb ─► MCP6002 ─► A0
```

## Notes

- A0 is the only analog input on ESP8266; the gate envelope must use it.
- With no fish, trim Rs so A0 sits mid-scale, then set `DETECT_DELTA` from the
  serial `Dev` readout (empty gap vs. fish in the gate).
- Both buttons are optional — count can also be toggled/reset via the web
  dashboard (`POST /api/toggle`, `POST /api/reset`).
- Detect deviation is |reading − baseline| > DETECT_DELTA in either direction;
  the baseline self-calibrates when the gate is clear.
- **IR sensor (reflective obstacle avoidance):** D8 (GPIO15). LOW = obstacle detected,
  HIGH = clear. Supply 3.3V–5V to module VCC. See `ir_sensor.md` for the TSOP38238
  beam-break alternative (different wiring, same pin).
