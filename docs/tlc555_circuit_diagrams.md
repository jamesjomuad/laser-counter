# TLC555 / 555 Timer Circuit Diagrams — GitHub Search Results

Searched: 2026-07-10

## In this repo (fish_counter)

| File | Description |
|---|---|
| `diagrams/tlc555_astable.svg` | TLC555 astable drive circuit (conductivity gate AC drive) |
| `diagrams/tlc555_astable_v2.svg` | Revised version of the astable circuit |
| `diagrams/tlc555.jpg` | Photo of the built circuit |
| `diagrams/ne555_astable.svg` | NE555 alternative astable reference |
| `diagrams/resistivity_sensor.md` | Full resistivity sensor hardware description + BOM |

## External GitHub repos — TLC555 / 555 conductivity & fish counter projects

| Repo | Relevance | Schematic format |
|---|---|---|
| [OpenWaterProject/riffle_328-conductivity](https://github.com/OpenWaterProject/riffle_328-conductivity) | DIY conductivity sensor — 555-based AC drive for water conductivity. Closest match to this project. Discusses probe geometry, fringing effects, 2-pin vs 4-pin, temp compensation | Fritzing / PDF |
| [cyberplantru/EC-Mini-v30-Sample-Code](https://github.com/cyberplantru/EC-Mini-v30-Sample-Code) | EC Mini v3.0 — NE555-based conductivity circuit for Arduino. AC excitation, frequency output (240 Hz dry → 8950 Hz at 80 mS/cm). Interrupt-based measurement. Range 0-160 mS/cm, ±2% accuracy | PCB + sample code |
| [wvmarle/Arduino_ECSensor](https://github.com/wvmarle/Arduino_ECSensor) | Simple EC sensor library for Arduino. Uses capacitor discharge through probe via 555-like AC method. Only 3 external components. Range 0.01-5 mS/cm (freshwater fish range) | Schematics + Arduino library |
| [OPEnSLab-OSU/ECSense](https://github.com/OPEnSLab-OSU/ECSense) | 4-wire EC meter shield for Adafruit Feather. Uses AC excitation with ADS1115 ADC. Supports temp compensation | Eagle PCB + code |
| [feslab/conduino](https://github.com/feslab/conduino) | Open-source conductivity sensor using micro USB connectors. 2-electrode design (revised from 4-electrode). Published in Sensors & Actuators B | Eagle + paper |
| [badTastingBread/cheapo-EC-TDS-meter](https://github.com/badTastingBread/cheapo-EC-TDS-meter) | Simple 3$ EC/TDS meter. Stainless steel rods, voltage divider + DS18B20 temp sensor. Accounts for electrolysis by pulsing measurement | Fritzing + Arduino code |
| [rohitbams/Plant-Synthesiser](https://github.com/rohitbams/Plant-Synthesiser) | 555 timer astable used as galvanic conductance sensor — plant tissue acts as variable resistor in timing network, Arduino reads pulse width via interrupt | Code + schematic |
| [dwblair/riffle-depth](https://github.com/dwblair/riffle-depth) | 555 timer used as capacitive depth/level sensor for water. 555 charges a wire pair capacitor, period proportional to submerged length | Schematic + Arduino code |
| [jbates35/fishCenS](https://github.com/jbates35/fishCenS) | Fish counting system for tracking salmon upstream. Different approach (likely optical/video) but same application domain | — |
| [Picatout/555_fading_led](https://github.com/Picatout/555_fading_led) | Dual TLC555 astable circuit — phase-slipping between two oscillators for LED fade effect. Good TLC555 dual-use reference | PNG |
| [BavinnK/555TimerBaby](https://github.com/BavinnK/555TimerBaby) | NE555 astable breadboard layout, scope waveforms, formula breakdown. Adjustable frequency/duty via two pots | Markdown + photos |
| [PAminai/555-timer-in-Astable-mode](https://github.com/PAminai/555-timer-in-Astable-mode) | NE555 astable PCB design with Proteus simulation | PDF + Proteus |
| [JeffJetton/555-timer-sim](https://github.com/JeffJetton/555-timer-sim) | Python simulation of 555 astable circuit | Python + markdown |
| [carwe/caphumidsens](https://github.com/carwe/caphumidsens) | NE555/TLC555 soil moisture sensor — capacitance → frequency (74 kHz dry → 4 kHz in water). ATtiny + NE555 alternative using comparator | Eagle files |

## Other web resources

- [Codrey Electronics — An Easy 555 EC Sensor](https://www.codrey.com/electronic-circuits/an-easy-555-ec-sensor/) — 555-based electrical conductivity sensor tutorial
- [TI TLC555 datasheet](https://www.ti.com/lit/ds/symlink/tlc555.pdf) — official datasheet with astable circuit diagram
