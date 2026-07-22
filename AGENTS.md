# Agent Notes — Fish Counter

## What this repo is
ESP8266/NodeMCU Arduino sketch that counts small tilapia fingerlings (~4 cm) passing through a 12 mm water-filled tube. Sensing is done with a **conductivity (resistivity) gate** (two stainless electrodes + TLC555 AC drive + MCP6002 buffer into A0), not a laser beam. The old laser/KY-008 wiring diagram in `diagrams/diagram.md` is stale — use `docs/diagrams/pins.md` and `docs/diagrams/resistivity_sensor.md` for the real hardware and BOM. An **alternative IR beam-break** approach using a TSOP38238 receiver is documented in `docs/diagrams/ir_sensor.md`.

## Build / upload
Use the exact CLI path and board FQBN below (do not assume a globally installed `arduino-cli`):

```bash
# compile
~/bin/arduino-cli.exe compile --fqbn esp8266:esp8266:nodemcuv2 firsh_counter/firsh_counter.ino

# upload (replace COMx with the actual port)
~/bin/arduino-cli.exe upload --fqbn esp8266:esp8266:nodemcuv2 --port COMx firsh_counter/firsh_counter.ino
```

Required libraries (install via Arduino Library Manager if missing):
- `TM1637Display` by Avishay Orpaz
- `WiFiManager` by tzapu

Required libraries (clone from GitHub — not in Library Manager):
- `ESPAsyncTCP` by me-no-dev → `~/Documents/Arduino/libraries/ESPAsyncTCP`
- `ESPAsyncWebServer` by me-no-dev → `~/Documents/Arduino/libraries/ESPAsyncWebServer`

## Entrypoints
- `firsh_counter.ino` — main sketch (sensor loop, display, buzzer, buttons).
- `wifi_dashboard.h` / `wifi_dashboard.cpp` — AsyncWebServer dashboard + SSE + REST API.
- `wifi_setup.h` / `wifi_setup.cpp` — WiFiManager connection setup (separate TU to avoid enum conflict between ESPAsyncWebServer and ESP8266WebServer).

The `.ino` compiles together with `wifi_dashboard.cpp` and `wifi_setup.cpp` automatically; no separate library packaging is needed.

## Pin map (NodeMCU v3)
| Function | Pin | Notes |
|---|---|---|
| Conductivity gate envelope | A0 | only analog input on ESP8266 |
| TM1637 CLK | D1 (GPIO5) | |
| TM1637 DIO | D2 (GPIO4) | |
| Start/Stop button | D5 (GPIO14) | INPUT_PULLUP, press pulls LOW |
| Reset button | D6 (GPIO12) | INPUT_PULLUP, press pulls LOW |
| Active buzzer (+) | D7 (GPIO13) | HIGH to beep |
| IR sensor (reflective obstacle) | D8 (GPIO15) | LOW = obstacle detected |
| IR sensor (TSOP38238 OUT) | D8 (GPIO15) | Digital input, LOW = beam intact (IR mode only) |

Power: LM2596 buck set to 5 V → NodeMCU VIN. TLC555 + MCP6002 run from NodeMCU 3V3.

## Key runtime behavior
- On first boot WiFiManager creates AP `FishCounter-Setup`; credentials persist in flash.
- Holding the reset button at boot clears saved WiFi credentials.
- Detection is `|reading − baseline| > DETECT_DELTA` in either direction; the baseline self-calibrates when the gate is clear.
- Dashboard at `http://<device-ip>/`; APIs: `GET /api/status`, `POST /api/toggle`, `POST /api/reset`, `GET /api/logs`.
- After the boot countdown, the TM1637 display cycles through each IP octet twice (e.g. `192.` `168.` `  1.` `100`) before showing the count. If WiFi isn't connected, it skips straight to the count.

## Key constants (in `firsh_counter.ino`)
| Constant | Value | Purpose |
|---|---|---|
| `DETECT_DELTA` | 60 | Min deviation from baseline to count a fish |
| `DEBOUNCE_MS` | 200 | Min ms between counts |
| `REARM_MS` | 80 | Gap must read clear this long before re-arming |
| `BASE_ALPHA` | 0.02 | Baseline adaptation rate (tracks water temp/salinity drift) |
| `BEEP_MS` | 50 | Buzzer beep duration per count |

## Tuning
1. With clear water/no fish, trim `Rs` so A0 sits mid-scale.
2. Watch serial output (`115200 baud`) for `Sensor / Baseline / Dev`.
3. Set `DETECT_DELTA` between empty-gap and fish-in-gap deviation.

## Conventions
- Variables: `camelCase`; constants/defines: `UPPER_SNAKE_CASE`.
- Section comments use `// ── Name ──` dividers.
- Serial debug at 115200 baud.

## No CI / tests
This repo has no unit tests, lint, formatter, or CI. Verification is compile → upload → observe serial output/hardware behavior.
