# Circuit Wiring Guide

## Components Required

| Component           | Qty | Purpose                              |
|---------------------|-----|--------------------------------------|
| NodeMCU ESP8266     | 1   | Microcontroller + WiFi               |
| DHT11 Sensor        | 1   | Temperature & Humidity sensing       |
| DC Fan (5V/12V)     | 1   | Cooling output                       |
| NPN Transistor (2N2222 or TIP120) | 1 | Fan speed control via PWM     |
| LDR (Photoresistor) | 1   | Light level sensing                  |
| Active Buzzer       | 1   | High-temperature alarm               |
| LED (Blue/White)    | 1   | Cold temperature indicator           |
| 10kΩ Resistor       | 2   | Pull-down for LDR + Base resistor    |
| 1kΩ Resistor        | 1   | LED current limiting                 |
| Diode 1N4007        | 1   | Flyback protection for fan motor     |
| Breadboard + Wires  | —   | Prototyping                          |

---

## Pin Connections

```
NodeMCU ESP8266
────────────────────────────────────────────────────────
 D4 (GPIO2)   ──────────────────── DHT11 DATA pin
              (DHT11: VCC→3.3V, GND→GND)

 D1 (GPIO5)   ─── 1kΩ ──────────── Base of NPN transistor
              Transistor Collector → Fan (–) terminal
              Transistor Emitter  → GND
              Fan (+)             → 5V / External supply
              1N4007 diode across fan terminals (cathode to +)

 A0           ─── LDR ────────────  3.3V
              A0 also connected to GND via 10kΩ (voltage divider)

 D5 (GPIO14)  ──────────────────── Buzzer (+)
              Buzzer (–)          → GND

 D6 (GPIO12)  ─── 220Ω ──────────── LED Anode (+)
              LED Cathode (–)     → GND

 3.3V         ──────────────────── DHT11 VCC
 GND          ──────────────────── DHT11 GND, Buzzer GND, LED GND
────────────────────────────────────────────────────────
```

---

## Schematic (ASCII)

```
3.3V ──┬──── DHT11 VCC          5V/External ──── Fan(+)
       │                                            │
       ├──── LDR ──── A0             1N4007 diode  │ (flyback)
       │      │                                    │
       │     10kΩ                          Fan(–) ──── Collector
       │      │                                        │  NPN
      GND    GND     D1 ──── 1kΩ ──── Base         Emitter
                                                        │
             D4 ──── DHT11 DATA                       GND

             D5 ──── Buzzer(+) ──── Buzzer(–) ──── GND

             D6 ──── 220Ω ──── LED(+) ──── LED(–) ──── GND
```

---

## Required Arduino Libraries

Install these via Arduino IDE → Tools → Manage Libraries:

| Library           | Author         | Version  |
|-------------------|----------------|----------|
| DHT sensor library| Adafruit       | ≥ 1.4.4  |
| ArduinoJson       | Benoit Blanchon| ≥ 6.21   |
| ESP8266WiFi       | (Built-in with ESP8266 board package) |
| ESP8266HTTPClient | (Built-in with ESP8266 board package) |

### ESP8266 Board Package
In Arduino IDE → File → Preferences → Additional Boards Manager URLs, add:
```
http://arduino.esp8266.com/stable/package_esp8266com_index.json
```
Then: Tools → Board Manager → search "esp8266" → Install.

Select board: **NodeMCU 1.0 (ESP-12E Module)**

---

## Fan Control Logic

| Temperature Range  | Fan Speed  | PWM Value (0–255) |
|--------------------|------------|-------------------|
| < 22°C             | 0% (Off)   | 0                 |
| 22°C – 45°C        | 0% → 100%  | Linear 0 → 255    |
| > 45°C             | 100% (Max) | 255               |

---

## Alert Conditions

| Condition          | Output        | Trigger           |
|--------------------|---------------|-------------------|
| Temperature > 60°C | Buzzer ON     | Alarm             |
| Temperature < 18°C | Cold LED ON   | Cold indicator    |

---

## Power Notes

- NodeMCU runs on **3.3V logic** — do not connect 5V signals to GPIO pins
- For fans drawing > 200mA, use an external power supply and transistor/MOSFET
- TIP120 Darlington transistor is recommended for higher-current fans
