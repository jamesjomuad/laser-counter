# Resistivity Fish-Counter Sensor (Design)

Replacement for the laser/LDR beam-break sensor. Counts **small tilapia
fingerlings** as they pass through a transparent transfer tube, using an
electrical **conductivity (resistivity) gate** instead of optics.

## Why not the laser

A water-filled transparent tube refracts the beam (acts as a cylindrical
lens), water absorbs infrared, and fingerlings are semi-transparent — so a
single-point laser/LDR reads unreliably. The resistivity gate senses the
whole cross-section, ignores water clarity and lighting, and is the proven
approach used by commercial aquaculture pipe counters.

## Locked parameters

| Parameter | Value |
|---|---|
| Tube inner diameter | 12 mm (bore doubles as the counting throat) |
| Fish | small tilapia fingerlings, ~4 cm |
| Water | freshwater |
| Electrode pairs | 1 (count only, no direction sensing) |
| AC excitation source | TLC555 astable (~2 kHz) |

Freshwater note: fish-vs-water contrast is weaker than in saltwater, so the
12 mm bore is kept snug (fish nearly fills it) and gain can be added if the
signal swing is too small.

## Electrodes

- Two **316 stainless-steel rings**, ~3–4 mm wide, set flush into the bore.
- **Gap ≈ 10 mm** between them (about one bore diameter).
- A fingerling spanning the gap modulates the water resistance → a pulse.
- 316 stainless + AC drive = no corrosion and no electrolysis.

## Signal chain (into A0)

```
  TLC555 ~2kHz       Cb        Rs(trim)        SENSE      envelope
  square wave ──┤1µF├──┬──[ 10k ]──┬── node ──►│┤├─►┬───► A0 (via C_A0 100 nF)
                        │           │           BAT43 │
                   Electrode A ◗    │              Ce 1 µF
                        )  ~10mm water gap         + Rb 10 k
                   Electrode B ◖    │               │
                        │          GND             GND
                    ┤1µF├
                     Cb │
                       GND
```

**Operation:** the TLC555 drives pure AC across the water gap (the 1 µF caps
block DC so the steel never corrodes). The water gap + Rs form an AC divider;
a fish changes the gap resistance, shifting the AC amplitude at **SENSE**.
The BAT43 + 1 µF + 10 k rectify that to a smooth DC level (~10 ms
response — fast enough for a fish in the throat for 50–200 ms). The 100 nF
cap at A0 filters noise. Detection is on *deviation from baseline*, so it
works whether a fish raises or lowers the level.

> **Without a buffer (MCP6002):** Rb must be lowered to 10 kΩ and Ce raised to
> 1 µF so the output impedance drops from 100 kΩ → 10 kΩ (the ESP8266 ADC is
> noisy with high-impedance sources). If you have an MCP6002, restore Ce/Rb to
> 100 nF / 100 k and insert it as a unity-gain buffer between envelope and A0.

## Bill of materials

| Part | Value / Type | Role |
|---|---|---|
| TLC555 (CMOS 555) | timing: R1 1 k, R2 33 k, Ct 10 nF | ~2 kHz AC source @ 3V3 |
| Rs | 10 k trimpot | set divider to mid-scale (tuning) |
| Cb ×2 | 1 µF (film/ceramic) | DC-block — protects electrodes |
| BAT43 (or 1N5819) | Schottky diode | rectifier (low drop, small-signal) |
| Ce / Rb | 1 µF / 10 kΩ | envelope smoothing (~10 ms), 10k output impedance |
| MCP6002 | rail-to-rail op-amp @ 3V3 | buffer into A0 (optional, recommended if available) |
| C_A0 | 100 nF ceramic | noise filter, solder direct across A0 to GND |
| Electrodes | 2× 316 SS rings | the sensing gate |

Power the TLC555 and op-amp from **3V3** so the level into A0 stays safe.
A0 is reused for conductivity; the KY-008 laser and KY-018 LDR are removed.
Display (D1/D2), buzzer (D7), and buttons (D5/D6) are unchanged.

## Firmware changes (planned)

- Replace `light > THRESHOLD` with `|reading − baseline| > DETECT_DELTA`,
  using a self-calibrating baseline (auto-tracks water temperature / salinity
  drift) and a sustained-clear re-arm so one fish isn't double-counted.
- Faster sampling and a serial print of `sensor / baseline / deviation` for
  live tuning of `DETECT_DELTA`.
- Unchanged: TM1637 display, buzzer, WiFi dashboard, reset button.

## Bring-up / tuning

1. Fill the tube with the actual water; with **no fish**, adjust the Rs
   trimpot so A0 sits mid-scale.
2. Watch the serial `deviation` value: empty tube vs. a fingerling in the gap.
3. Set `DETECT_DELTA` between those two levels.
4. If the swing is too small, add gain to the MCP6002 (two resistors) to
   amplify before the ADC.
