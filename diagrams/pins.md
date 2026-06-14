# Fish Counter — Wiring Diagram

Sensor is a **conductivity (resistivity) gate**, not a laser beam. Full
electrode + analog front-end design is in `resistivity_sensor.md`; this file
covers the NodeMCU pin connections.

## NodeMCU v3 (ESP8266) Pin Map

| Component | Pin | NodeMCU Pin | GPIO | Notes |
|---|---|---|---|---|
| Conductivity gate (envelope) | OUT | A0 | ADC0 | Rectified gate level; only analog input on ESP8266 |
| TM1637 Display | CLK | D1 | GPIO5 | Clock line |
| TM1637 Display | DIO | D2 | GPIO4 | Data line |
| Start/Stop Button | Leg 1 | D5 | GPIO14 | INPUT_PULLUP; press pulls LOW |
| Reset Button | Leg 1 | D6 | GPIO12 | INPUT_PULLUP; press pulls LOW |
| Active Buzzer | + | D7 | GPIO13 | Driven HIGH to beep |
| Buttons / Buzzer | Leg 2 / − | GND | — | Ground |

The analog front end (TLC555 AC source, electrodes, BAT43 envelope detector,
MCP6002 buffer) feeds the single A0 line. Power the 555 and op-amp from 3V3.

## Power

- LM2596 buck converter set to 5V output → NodeMCU VIN + GND
- TLC555 and MCP6002 run off the board's 3.3V rail
- Electrodes are AC-driven (DC-blocked) so the stainless never corrodes

## ASCII Wiring

```
                        NodeMCU v3
                     +--------------+
                     |              |
  Gate envelope ---→| A0           |   (from MCP6002 buffer)
                     |              |
                     |  D1 (GPIO5) |←------ TM1637 CLK
                     |  D2 (GPIO4) |←------ TM1637 DIO
                     |              |
                     |  D5 (GPIO14)|←------ Start/Stop Button ─→ GND
                     |  D6 (GPIO12)|←------ Reset Button ──────→ GND
                     |  D7 (GPIO13)|←------ Buzzer (+)
                     |              |
                     |          3V3 |──→ TLC555 + MCP6002 VCC
                     |          GND |──→ Buzzer (-), Buttons, analog GND
                     |          VIN |←── LM2596 5V out
                     +--------------+

  Analog front end (see resistivity_sensor.md):
    TLC555 ~2kHz ─┤Cb├─ Rs ─┬─ Electrode A )(  water gap  )( Electrode B ─┤Cb├─ GND
                            └─ SENSE ─► BAT43 + 100nF/100k ─► MCP6002 ─► A0
```

## Notes

- A0 is the only analog input on ESP8266; the gate envelope must use it.
- With no fish, trim Rs so A0 sits mid-scale, then set `DETECT_DELTA` from the
  serial `Dev` readout (empty gap vs. fish in the gate).
- Both buttons are optional — count can also be toggled/reset via the web
  dashboard (`POST /api/toggle`, `POST /api/reset`).
