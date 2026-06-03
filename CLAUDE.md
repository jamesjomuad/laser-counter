# Laser Beam Counter

## Purpose
An Arduino-based object counter that detects when something breaks a laser beam. Uses a photoresistor (LDR) to sense beam interruption and displays the running count on a 4-digit 7-segment display.

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
| KY-008 laser emitter | Emits beam (always on) | S → 3V3 |
| KY-018 photoresistor | Detects beam break | S → A0 |
| TM1637 4-digit display | Shows count | CLK → D1, DIO → D2 |
| Active buzzer | Beeps on each count | + → D7, - → GND |
| Reset button (optional) | Resets count to 0 | D6 → GND (INPUT_PULLUP) |
| LM2596 buck converter | Power (set to 5V) | — |

## Project Structure
```
laser_counter/
  laser_counter.ino       — Main sketch (sensor, display, buzzer, loop)
  wifi_dashboard.h        — WiFi + web server interface
  wifi_dashboard.cpp      — WiFiManager setup, HTML dashboard, API handlers
  diagrams/
    pins.md               — ASCII wiring diagram and pin table
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
- Dashboard at `http://<device-ip>/` with live count, sensor bar, beam status, reset button
- API: `GET /api/status`, `POST /api/reset`

## Key Constants
- `THRESHOLD` (400) — Analog value above which beam is considered broken
- `DEBOUNCE_MS` (200) — Minimum ms between counts
- `BEEP_MS` (50) — Buzzer beep duration per count

## Conventions
- Core logic in `.ino`, WiFi/web split into `wifi_dashboard.h/.cpp`
- Section comments with `// ── Name ──` style dividers
- `camelCase` for variables, `UPPER_SNAKE_CASE` for constants/defines
- Serial debug output at 115200 baud
