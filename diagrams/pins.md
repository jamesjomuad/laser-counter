# Laser Counter — Wiring Diagram

## NodeMCU v3 (ESP8266) Pin Map

| Component | Pin | NodeMCU Pin | GPIO | Notes |
|---|---|---|---|---|
| KY-008 Laser | S | 3V3 | — | Always on, powered from 3.3V rail |
| KY-018 Photoresistor | S | A0 | ADC0 | Only analog input on ESP8266 |
| TM1637 Display | CLK | D1 | GPIO5 | Clock line |
| TM1637 Display | DIO | D2 | GPIO4 | Data line |
| Active Buzzer | + | D7 | GPIO13 | Driven HIGH to beep |
| Active Buzzer | - | GND | — | Ground |
| Reset Button | Leg 1 | D6 | GPIO12 | INPUT_PULLUP; press pulls LOW |
| Reset Button | Leg 2 | GND | — | Ground |

## Power

- LM2596 buck converter set to 5V output → NodeMCU VIN + GND
- KY-008 laser runs off the board's 3.3V rail
- KY-018 module has its own voltage divider — connect S→A0, +→3V3, -→GND

## ASCII Wiring

```
                        NodeMCU v3
                     +--------------+
                     |              |
  KY-018 (S) ------→| A0           |
                     |              |
                     |  D1 (GPIO5) |←------ TM1637 CLK
                     |  D2 (GPIO4) |←------ TM1637 DIO
                     |              |
                     |  D6 (GPIO12)|←------ Reset Button ──→ GND
                     |  D7 (GPIO13)|←------ Buzzer (+)
                     |              |
  KY-008 (S) ------→| 3V3          |
                     |          GND |──→ Buzzer (-), Button, KY-018 (-)
                     |          VIN |←── LM2596 5V out
                     +--------------+
```

## Notes

- Align the laser so it shines directly onto the photoresistor. A0 should
  read well below 400 with beam intact, above 400 when broken.
- Reset button is optional — count can also be reset via `POST /api/reset`.
- A0 is the only analog input on ESP8266; photoresistor must use it.
