# Bill of Materials (BOM) — ROBO Race Bot Wireless

This document lists the recommended components to build a high-performance, competition-ready wireless race bot.

---

## 1. Core Electronics

| Item | Component | Recommended Model / Spec | Qty | Purpose |
|---|---|---|---|---|
| **MCU** | ESP32 Microcontroller | ESP32 DevKit V1 (30-pin / 38-pin) | 1 | High-speed dual-core 240MHz MCU with 2.4GHz Wi-Fi, BLE, and ESP-NOW support |
| **Motor Driver** | Dual H-Bridge Driver | **TB6612FNG** (Best efficiency) or **L298N** (Classic) | 1 | Drives left and right motors with bidirectional PWM speed control |
| **Voltage Regulator** | Buck Converter | **Mini-360** or **LM2596** Step-Down DC-DC | 1 | Steps down 7.4V–12.6V battery voltage to stable 5.0V for the ESP32 |
| **Battery** | Rechargeable Power Pack | **2S (7.4V) or 3S (11.1V) LiPo** (1000–2200mAh, 25C+) or **2x 18650 Li-ion cells** | 1 | High discharge rate power supply for instant acceleration and turbo |
| **Power Switch** | SPST Rocker / Toggle Switch | 5A–10A rated heavy-duty switch | 1 | Master power disconnect for safety |

---

## 2. Drivetrain & Chassis

| Item | Component | Recommended Model / Spec | Qty | Purpose |
|---|---|---|---|---|
| **Motors** | DC Gear Motors | **12V 500–600 RPM Metal Gearmotors** (or N20 600RPM micro motors) | 2–4 | High speed + adequate torque for racing agility |
| **Wheels** | Racing Tires | 65mm High-Grip Rubber / Silicone Wheels | 2–4 | Maximum traction on tile, wood, or carpet tracks |
| **Chassis** | Frame Platform | Lightweight Acrylic, Carbon Fiber, or 3D Printed 2WD/4WD | 1 | Low center of gravity, aerodynamic chassis |
| **Front Support** | Ball Caster (if 2WD) | Metal ball caster wheel (low friction) | 1 | Smooth omnidirectional front/rear pivot |

---

## 3. Passive Components & Protection

| Item | Component | Spec | Qty | Purpose |
|---|---|---|---|---|
| **Capacitors** | Electrolytic Capacitor | 470µF – 1000µF (16V–25V) | 1 | Parallel across motor power rail to absorb motor voltage spikes |
| **Decoupling** | Ceramic Capacitor | 100nF (0.1µF) | 2–4 | Solder across motor terminals (+ and -) to suppress RF noise |
| **Voltage Divider** | Resistors (ADC Battery Monitor) | 100kΩ & 22kΩ (1% metal film) | 1 pair | Steps down 12V battery voltage to <3.3V for ESP32 ADC pin (GPIO 34) |
| **Status LED / Buzzer** | Active Buzzer & LED | 5V Active Buzzer + 5mm Bright LED | 1 ea | Horn sound effect & race status / headlight indicator |

---

## 4. Hardware Transmitter (Optional - For Dedicated Remote)

If using the dedicated **ESP-NOW Ultra-Low Latency Remote**:

| Item | Component | Recommended Model | Qty | Purpose |
|---|---|---|---|---|
| **MCU** | ESP32 DevKit | ESP32 DevKit V1 | 1 | Sends sub-5ms steering & throttle packets |
| **Joystick** | 2-Axis Analog Thumbstick | Dual-potentiometer PS2-style joystick module | 1 | Proportional throttle & steering |
| **Buttons** | Tactile Push Buttons | 12x12mm with caps | 2–4 | Turbo, Brake, Horn, Lights |
| **Display** | I2C OLED Screen | 0.96" SSD1306 128x64 OLED | 1 | Real-time telemetry (Battery, RSSI, Ping) |
| **Battery** | Portable Power | 1x 18650 cell with 3.7V to 5V boost module or 9V battery | 1 | Powering remote controller |

---

## 5. Summary of Recommended Specs

* **Weight Target:** Under 450g (for optimal acceleration and drift handling)
* **Operating Voltage:** 7.4V (2S LiPo) to 11.1V (3S LiPo)
* **Top Speed:** ~3.5 to 5.0 meters/second (approx. 15–18 km/h on 600 RPM / 65mm wheels)
* **Wireless Range:** ~50–80m (Wi-Fi AP mode), ~150–200m (ESP-NOW mode)
* **Control Latency:** <15ms (WebSocket Wi-Fi), <3ms (ESP-NOW direct link)
