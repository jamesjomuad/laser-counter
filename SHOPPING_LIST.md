# Fish Counter — Shopping Checklist

Checklist for buying components for the ESP8266/NodeMCU tilapia fingerling counter.
The current design uses a **conductivity (resistivity) gate**, not the old laser/LDR setup.

## Must-have for the current design

- [ ] **NodeMCU v3** (ESP8266, 30-pin)
- [ ] **TLC555CP** — CMOS 555 timer, 8-pin DIP  
  *(Alternatives: LMC555, ICM7555. Do **not** use a standard NE555 — it needs 4.5 V minimum and this circuit runs at 3.3 V.)*
- [ ] **MCP6002** rail-to-rail op-amp (8-pin DIP or SOIC)
- [ ] **TM1637 4-digit display**
- [ ] **LM2596 buck converter module** (adjustable; set output to 5 V)
- [ ] **Tactile pushbuttons** ×2 (start/stop + reset)
- [ ] **Active buzzer** 5 V or 3.3 V logic-level
- [ ] **10 kΩ trimpot / multi-turn preset** (for Rs tuning)
- [ ] **Resistors:** 1 kΩ, 33 kΩ, 100 kΩ
- [ ] **Capacitors:** 10 nF, 100 nF, 1 µF ×2
- [ ] **Schottky diode:** BAT43, BAT54, or 1N5819
- [ ] **316 stainless-steel rings** ×2 for electrodes  
  *(Or 316 stainless washers as a simpler alternative.)*
- [ ] **Clear tube** 12 mm inner diameter

## Useful add-ons

- [ ] **DS18B20 waterproof temperature sensor** + 4.7 kΩ resistor  
  *(For temperature compensation of the conductivity baseline.)*
- [ ] **0.96" I2C OLED** (SSD1306) — easier tuning than the 4-digit display
- [ ] **Micro-SD card module** + **DS3231 RTC** — for timestamped count logging
- [ ] **PPTC resettable fuse** 500 mA — short-circuit protection
- [ ] **1N5819 Schottky diode** — reverse-polarity protection on VIN
- [ ] **Breadboard + jumper wires** — for prototyping
- [ ] **IP65 enclosure + cable glands** — if the unit will be near splashing water

## What NOT to buy

- ❌ **NE555** — will not work at 3.3 V
- ❌ **KY-008 laser module** — obsolete for this design
- ❌ **KY-018 LDR module** — obsolete for this design
