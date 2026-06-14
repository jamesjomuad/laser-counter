# Fish Counter

## Purpose
An Arduino-based fish counter that counts small tilapia fingerlings as they pass through a 12 mm water-filled tube. A conductivity (resistivity) gate — two stainless electrodes in the bore — senses each fish via the change in water resistance, and the running count is shown on a 4-digit 7-segment display.

> Originally a laser/LDR beam-break counter; switched to a resistivity gate because a transparent, water-filled tube refracts the beam and semi-transparent fingerlings don't reliably break it. See `diagrams/resistivity_sensor.md` for the full sensor design.

## Tech Stack
- **Platform:** Arduino (ESP8266 / NodeMCU v3 with CH340 USB chip)
- **Language:** C++ (Arduino `.ino` sketch)
- **Libraries:**
  - TM1637Display by Avishay Orpaz
  - WiFiManager by tzapu
- **CLI:** `arduino-cli` installed at `~/bin/arduino-cli.exe`

## Hardware
| Component | Role | Connection |
|-----------|------|------------|
| Conductivity gate | Senses fish in the bore | electrodes → AC bridge → A0 |
| TLC555 AC source | ~2 kHz drive for the gate (no DC = no corrosion) | 3V3 |
| MCP6002 op-amp | Buffers the rectified envelope into A0 | 3V3 |
| TM1637 4-digit display | Shows count | CLK → D1, DIO → D2 |
| Active buzzer | Beeps on each count | + → D7, - → GND |
| Start/Stop button | Toggles counting | D5 → GND (INPUT_PULLUP) |
| Reset button (optional) | Resets count to 0 | D6 → GND (INPUT_PULLUP) |
| LM2596 buck converter | Power (set to 5V) | — |

Full electrode/circuit detail and BOM: `diagrams/resistivity_sensor.md`.

## Project Structure
```
laser_counter/
  laser_counter.ino       — Main sketch (sensor, display, buzzer, loop)
  wifi_dashboard.h        — WiFi + web server interface
  wifi_dashboard.cpp      — WiFiManager setup, HTML dashboard, API handlers
  diagrams/
    pins.md               — ASCII wiring diagram and pin table
    resistivity_sensor.md — Conductivity-gate sensor design + BOM
  CLAUDE.md               — This file
```

## Build & Upload
```bash
# Compile
~/bin/arduino-cli.exe compile --fqbn esp8266:esp8266:nodemcuv2 laser_counter.ino

# Upload (replace COMx with actual port)
~/bin/arduino-cli.exe upload --fqbn esp8266:esp8266:nodemcuv2 --port COMx laser_counter.ino
```

## WiFi Dashboard
- On first boot, creates AP **"LaserCounter-Setup"** — connect and enter WiFi credentials
- Credentials persist in flash across power cycles
- Dashboard at `http://<device-ip>/` with live count, sensor bar, gate status, start/stop + reset buttons, live log
- API: `GET /api/status`, `POST /api/reset`, `POST /api/toggle`, `GET /api/logs`

## Key Constants
- `DETECT_DELTA` (60) — Min deviation from the self-calibrating baseline to count a fish
- `BASE_ALPHA` (0.02) — Baseline adaptation rate (tracks water temp/salinity drift)
- `DEBOUNCE_MS` (200) — Minimum ms between counts
- `REARM_MS` (80) — Gate must read clear this long before the next count
- `BEEP_MS` (50) — Buzzer beep duration per count

## Conventions
- Core logic in `.ino`, WiFi/web split into `wifi_dashboard.h/.cpp`
- Section comments with `// ── Name ──` style dividers
- `camelCase` for variables, `UPPER_SNAKE_CASE` for constants/defines
- Serial debug output at 115200 baud
