# Arduino IDE Setup — NodeMCU v3 (ESP8266)

## 1. Install Arduino IDE

Download from https://www.arduino.cc/en/software

## 2. Add ESP8266 Board Support

1. Open Arduino IDE → **File → Preferences**
2. In **Additional Boards Manager URLs**, add:
   ```
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
   (Click OK)
3. **Tools → Board → Boards Manager**
4. Search `esp8266` → install **esp8266 by ESP8266 Community**

## 3. Select Board

**Tools → Board → ESP8266 Boards → NodeMCU 1.0 (ESP-12E Module)**

## 4. Install Required Libraries

**Tools → Manage Libraries** → install:

| Library | Author |
|---|---|
| `TM1637Display` | Avishay Orpaz |
| `WiFiManager` | tzapu |

## 5. Select Port

1. Plug in NodeMCU via USB
2. **Tools → Port** → select the COM port (e.g. COM3, COM5)
   - If no port appears, install CH340 driver: https://www.wch.cn/download/CH341SER_EXE.html

## 6. Open Sketch

**File → Open** → select `firsh_counter/firsh_counter.ino`

## 7. Compile & Upload

- **Verify** (✓) to compile
- **→** arrow to upload

## 8. First Boot — WiFi Setup

On first boot (or after clearing saved WiFi), the device creates an AP named **`FishCounter-Setup`**.

1. Connect your phone/PC to `FishCounter-Setup`
2. A captive portal should open — enter your WiFi credentials
3. Once connected, open a browser to the device IP shown in serial monitor
4. Dashboard at `http://<device-ip>/`

### Clearing saved WiFi credentials

Two methods:

| Method | How |
|---|---|
| **Hardware** — Hold D6 to GND while powering on / pressing RST | Hold D6 low → press RST → release D6 |
| **Serial** — Type `RESETWIFI` or `CLEARWIFI` in serial monitor | Open 115200 baud serial monitor → type command → board reboots as AP |

## Serial Commands

| Command | Action |
|---|---|
| `RESETWIFI` or `CLEARWIFI` | Wipes saved WiFi credentials and restarts (AP mode) |

## Pin Mapping (NodeMCU)

| Label | GPIO | Function |
|---|---|---|
| A0 | — | Conductivity gate envelope |
| D1 | GPIO5 | TM1637 CLK |
| D2 | GPIO4 | TM1637 DIO |
| D5 | GPIO14 | Start/Stop button (INPUT_PULLUP) |
| D6 | GPIO12 | Reset button (INPUT_PULLUP) |
| D7 | GPIO13 | Active buzzer |

## Troubleshooting

| Problem | Fix |
|---|---|
| No COM port | Install CH340 driver (link above) |
| `TM1637Display` not found | Install via Library Manager |
| `WiFiManager` not found | Install via Library Manager |
| Upload fails — `espcomm_sync` error | Hold RST, press upload, release RST when connecting |
| Upload fails — wrong board | Ensure **NodeMCU 1.0 (ESP-12E Module)** is selected |
| Can't see WiFi AP `FishCounter-Setup` | Device may have saved credentials from a previous network. Clear WiFi via serial (`RESETWIFI`) or hold D6 low while pressing RST |
| Serial command not working | Ensure baud rate is **115200** and line ending is set to **Newline** (`\n`) in serial monitor |
