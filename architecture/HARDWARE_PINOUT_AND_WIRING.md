# Hardware Pinout & Wiring Specifications

This document defines the complete electrical pin mappings, voltage divider calculations, and sensor circuits for both the **Bot Receiver (ESP32-S3)** and the **Handheld Remote (Standard ESP32)**.

---

# Part 1: Robot Receiver (ESP32-S3-WROOM-1U-N16)

The bot uses the compact SMD module with an external **U.FL antenna** and **native USB-C** debugging/flashing.

## 1.1 Complete GPIO Pin Assignment Table

| Pin No. | Label | Connection Target | Circuit / Peripheral | Notes |
|---|---|---|---|---|
| **Pin 1** | `GND` | Power GND Bus | Common Ground Reference | Shared across battery, buck, drivers |
| **Pin 2** | `3V3` | Buck Converter Output | Clean 3.3V Logic Supply ($\ge 500\text{mA}$) | Add 10µF + 100nF decoupling caps |
| **Pin 3** | `EN` | Reset Line | 10kΩ Pull-Up to 3.3V + 1µF Cap to GND | Power-on clean reset circuit |
| **Pin 4** | `GPIO 4` | Cell 1 Voltage Divider | `ADC1_CH3` Analog Input | 10kΩ / 10kΩ divider (0–4.2V $\rightarrow$ 0–2.1V) |
| **Pin 5** | `GPIO 5` | Cell 2 Voltage Divider | `ADC1_CH4` Analog Input | 30kΩ / 10kΩ divider (0–8.4V $\rightarrow$ 0–2.1V) |
| **Pin 6** | `GPIO 6` | Cell 3 Voltage Divider | `ADC1_CH5` Analog Input | 50kΩ / 10kΩ divider (0–12.6V $\rightarrow$ 0–2.1V) |
| **Pin 7** | `GPIO 7` | Left Driver `L_IS` | `ADC1_CH6` Analog Input | Current sense ($1\text{ k}\Omega$ shunt to GND) |
| **Pin 8** | `GPIO 8` | Right Driver `R_IS` | `ADC1_CH7` Analog Input | Current sense ($1\text{ k}\Omega$ shunt to GND) |
| **Pin 13** | `GPIO 19` | USB Type-C `D-` | Native USB 2.0 Full-Speed Data - | Built-in USB Serial/JTAG controller |
| **Pin 14** | `GPIO 20` | USB Type-C `D+` | Native USB 2.0 Full-Speed Data + | Built-in USB Serial/JTAG controller |
| **Pin 22** | `GPIO 14` | DS18B20 1-Wire Data | 3x Digital Temperature Sensors | $4.7\text{ k}\Omega$ pull-up to 3.3V |
| **Pin 27** | `GPIO 0` | Boot Strapping Pin | 10kΩ Pull-Up to 3.3V | Ensures normal SPI flash boot |
| **Pin 10** | `GPIO 17` | Left BTS7960 `RPWM` | Hardware MCPWM Channel 0 (20 kHz) | Left Motor Forward Speed |
| **Pin 11** | `GPIO 18` | Left BTS7960 `LPWM` | Hardware MCPWM Channel 1 (20 kHz) | Left Motor Reverse Speed |
| **Pin 8** | `GPIO 15` | Left BTS7960 `EN` | Digital Output (High = Active) | Bridges `R_EN` and `L_EN` together |
| **Pin 31** | `GPIO 38` | Right BTS7960 `RPWM` | Hardware MCPWM Channel 2 (20 kHz) | Right Motor Forward Speed |
| **Pin 32** | `GPIO 39` | Right BTS7960 `LPWM` | Hardware MCPWM Channel 3 (20 kHz) | Right Motor Reverse Speed |
| **Pin 33** | `GPIO 40` | Right BTS7960 `EN` | Digital Output (High = Active) | Bridges `R_EN` and `L_EN` together |
| **Pin 40** | `GND` | Power GND Bus | Additional Ground Pad | Solder directly to GND plane |
| **Pin 41** | `EPAD` | Thermal Ground | Exposed Bottom Ground Pad | Heatsink dissipation |

---

## 1.2 Sensor Circuits

### A. 3S Balance Port Voltage Dividers
```
 Pin 2 (Cell 1: 0 - 4.2V)  ----[ 10kΩ ]----+----[ 10kΩ ]----> GND
                                           |
                                           +---> GPIO 4 (ADC1_CH3)

 Pin 3 (Cell 2: 0 - 8.4V)  ----[ 30kΩ ]----+----[ 10kΩ ]----> GND
                                           |
                                           +---> GPIO 5 (ADC1_CH4)

 Pin 4 (Cell 3: 0 - 12.6V) ----[ 50kΩ ]----+----[ 10kΩ ]----> GND
                                           |
                                           +---> GPIO 6 (ADC1_CH5)
```

### B. Motor Current Sense Circuit
```
 BTS7960 (R_IS / L_IS) ----+----[ 1 kΩ Shunt Resistor ]----> Common GND
                           |
                           +---> ESP32-S3 GPIO 7 / GPIO 8 (ADC1)
```
* At $15\text{A}$ (Full 2-motor stall): $V_{out} = \frac{15\text{A}}{8500} \times 1000\Omega \approx 1.76\text{V}$ (Safely within 3.3V limit).

### C. 3x DS18B20 1-Wire Digital Temperature Bus
```
 ESP32-S3 (GPIO 14) ----------------+------------------+------------------+
                                    |                  |                  |
                               [ SENSOR 1 ]       [ SENSOR 2 ]       [ SENSOR 3 ]
                             (3S LiPo Battery)   (Left BTS7960)     (Right BTS7960)
 3.3V Power -------[ 4.7kΩ ]--------+                  |                  |
 GND -------------------------------+------------------+------------------+
```

---

# Part 2: Handheld Remote Controller (ESP32 Standard)

The remote uses a standard **ESP32 DevKit V1 (30-pin or 38-pin)** with tactile switches, an I2C OLED, and a rotary encoder.

## 2.1 Complete GPIO Pin Assignment Table

| Remote GPIO | Device | Function | Wiring Type |
|---|---|---|---|
| **GPIO 13** | Upper Left Button | **Left Forward** | Pushbutton to GND (`INPUT_PULLUP`) |
| **GPIO 12** | Lower Left Button | **Left Reverse** | Pushbutton to GND (`INPUT_PULLUP`) |
| **GPIO 14** | Upper Right Button | **Right Forward** | Pushbutton to GND (`INPUT_PULLUP`) |
| **GPIO 27** | Lower Right Button | **Right Reverse** | Pushbutton to GND (`INPUT_PULLUP`) |
| **GPIO 32** | Trigger 1 Button | **Speed Preset 1 Override** | Pushbutton to GND (`INPUT_PULLUP`) |
| **GPIO 33** | Trigger 2 Button | **Speed Preset 2 Override** | Pushbutton to GND (`INPUT_PULLUP`) |
| **GPIO 25** | EC11 Rotary Encoder | `CLK` (Phase A) | Encoder pin with 10k pull-up |
| **GPIO 26** | EC11 Rotary Encoder | `DT` (Phase B) | Encoder pin with 10k pull-up |
| **GPIO 4**  | EC11 Rotary Encoder | `SW` (Push Switch) | Encoder click button (`INPUT_PULLUP`) |
| **GPIO 21** | 0.96" OLED Display | `SDA` (I2C Data) | Standard I2C Data line |
| **GPIO 22** | 0.96" OLED Display | `SCL` (I2C Clock) | Standard I2C Clock line |
| **3.3V**    | Display & Encoder | Logic Power Supply | Powers OLED and Encoder VCC |
| **GND**     | All Peripherals | Common Ground | Common return path |

---

## 2.2 Remote Ergonomics Map

```
                          [ 0.96" I2C OLED ]
                       [ EC11 ROTARY ENCODER ]

      LEFT THUMB                                      RIGHT THUMB
 +------------------+                            +------------------+
 |  [ GPIO 13 ]     | (Left Fwd)      (Right Fwd)|  [ GPIO 14 ]     |
 |  [ GPIO 12 ]     | (Left Rev)      (Right Rev)|  [ GPIO 27 ]     |
 +------------------+                            +------------------+

     LEFT INDEX FINGER                              RIGHT INDEX FINGER
 +-----------------------+                      +-----------------------+
 | [ GPIO 32: TRIG 1 ]   |                      | [ GPIO 33: TRIG 2 ]   |
 | Speed Preset 1 (500)  |                      | Speed Preset 2 (600)  |
 +-----------------------+                      +-----------------------+
```
