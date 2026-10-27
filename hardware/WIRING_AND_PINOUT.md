# Wiring Guide & Pinout Diagram — ROBO Race Bot Wireless

This guide provides clear schematics, pin connections, and power distribution rules for building the **ROBO Race Bot Wireless**.

---

## 1. ESP32 Pin Mapping Table

| ESP32 GPIO | Function | Connects To (TB6612FNG) | Connects To (L298N) | Notes |
|---|---|---|---|---|
| **GPIO 18** | Left Motor PWM | `PWMA` | `ENA` (Remove Jumper) | PWM Channel 0 (10 kHz, 8-bit) |
| **GPIO 19** | Left Motor DIR 1 | `AIN1` | `IN1` | Logic HIGH/LOW for direction |
| **GPIO 21** | Left Motor DIR 2 | `AIN2` | `IN2` | Logic HIGH/LOW for direction |
| **GPIO 22** | Right Motor DIR 1| `BIN1` | `IN3` | Logic HIGH/LOW for direction |
| **GPIO 23** | Right Motor DIR 2| `BIN2` | `IN4` | Logic HIGH/LOW for direction |
| **GPIO 25** | Right Motor PWM | `PWMB` | `ENB` (Remove Jumper) | PWM Channel 1 (10 kHz, 8-bit) |
| **GPIO 5**  | Driver Standby  | `STBY` (HIGH=Active)   | *(N/A for L298N)*   | Set HIGH to enable TB6612FNG |
| **GPIO 34** | Battery Voltage | Voltage Divider Center | Voltage Divider Center | Analog Input (ADC1) — safe with Wi-Fi |
| **GPIO 2**  | Headlight / LED | LED Anode (+ via 220Ω) | LED Anode (+ via 220Ω) | On-board / External LED indicator |
| **GPIO 4**  | Horn / Buzzer   | Active Buzzer (+)      | Active Buzzer (+)      | Sound effects on button press |

> [!IMPORTANT]
> Always use **ADC1 pins** (GPIO 32–39) for analog readings when Wi-Fi is active. ADC2 pins (GPIO 0, 2, 4, 12, 13, 14, 15, 25, 26, 27) cannot be used for analog reads while Wi-Fi is running on the ESP32. We use **GPIO 34**.

---

## 2. Power Architecture Diagram

```
                 +-------------------------------+
                 |  LiPo Battery (7.4V - 11.1V)  |
                 +---------------+---------------+
                                 |
                      [ Master Power Switch ]
                                 |
        +------------------------+------------------------+
        |                                                 |
        v                                                 v
+------------------+                             +--------------------+
| Buck Converter   |                             | Motor Driver VM    |
| (LM2596/Mini360) |                             | (TB6612: VM /      |
| Output: 5.0V     |                             |  L298N: 12V In)    |
+--------+---------+                             +---------+----------+
         |                                                 |
         v 5.0V                                            v
+------------------+                             +--------------------+
| ESP32 DevKit     |                             | DC Motors          |
| (VIN / 5V Pin)   |                             | Left & Right       |
+--------+---------+                             +---------+----------+
         |                                                 |
         +------------------- Common GND ------------------+
                                (Crucial!)
```

### Critical Rules:
1. **Common Ground:** The ESP32 `GND`, Motor Driver `GND`, and Battery `GND` **MUST** be connected together. Without a shared ground reference, logic signals will float and motors will twitch uncontrollably.
2. **Never Power Motors from ESP32:** Do **NOT** connect motor positive leads to the ESP32 3.3V or 5V/VIN pins. Motors draw surge currents (up to 2–5 Amps during stall or fast acceleration) that will instantly reboot or destroy the microcontroller.
3. **Standby Pin (TB6612FNG):** The `STBY` pin must receive 3.3V or be pulled HIGH via GPIO 5 to take the driver out of sleep mode.

---

## 3. Motor Driver Comparison & Connections

### Option A: TB6612FNG (Recommended — High Efficiency, MOSFET based)

```
       TB6612FNG                     ESP32 / Power
   +---------------+
   | VM            |<---- Battery V+ (7.4V - 12V) [With 470uF Cap to GND]
   | VCC           |<---- ESP32 3.3V (Logic Power)
   | GND           |<---- Common Ground
   | A01, A02      |----> Left Motor Terminals
   | B01, B02      |----> Right Motor Terminals
   | PWMA          |<---- ESP32 GPIO 18 (PWM)
   | AIN1          |<---- ESP32 GPIO 19
   | AAIN2         |<---- ESP32 GPIO 21
   | STBY          |<---- ESP32 GPIO 5 (or wire to 3.3V)
   | BIN1          |<---- ESP32 GPIO 22
   | BIN2          |<---- ESP32 GPIO 23
   | PWMB          |<---- ESP32 GPIO 25 (PWM)
   +---------------+
```

### Option B: L298N Dual H-Bridge

```
       L298N Module                  ESP32 / Power
   +---------------+
   | 12V Power In  |<---- Battery V+ (7.4V - 12V)
   | GND           |<---- Common Ground
   | 5V In / Out   |<---- Disconnect jumper if battery > 12V
   | ENA           |<---- ESP32 GPIO 18 (PWM) [Remove black jumper]
   | IN1           |<---- ESP32 GPIO 19
   | IN2           |<---- ESP32 GPIO 21
   | IN3           |<---- ESP32 GPIO 22
   | IN4           |<---- ESP32 GPIO 23
   | ENB           |<---- ESP32 GPIO 25 (PWM) [Remove black jumper]
   | OUT1, OUT2    |----> Left Motor Terminals
   | OUT3, OUT4    |----> Right Motor Terminals
   +---------------+
```

---

## 4. Battery Voltage Monitoring Circuit (GPIO 34)

Protects LiPo batteries from over-discharge (dropping below 3.3V/cell can permanently damage LiPo batteries).

```
   Battery Positive (Vbat: 7.4V - 12.6V)
             |
             |
           [ R1: 100 kΩ ]
             |
             +---------> Connect to ESP32 GPIO 34 (ADC1)
             |
           [ R2: 22 kΩ ]
             |
             |
        Common GND
```

**Calculation:**
* $V_{out} = V_{bat} \times \frac{R_2}{R_1 + R_2} = V_{bat} \times \frac{22}{122} \approx V_{bat} \times 0.1803$
* At maximum 3S charge ($12.6\text{ V}$), $V_{out} = 12.6 \times 0.1803 = 2.27\text{ V}$ (Well within ESP32 ADC $0–3.3\text{ V}$ range).
* Firmware formula:
  $$\text{Vbat} = \left(\frac{\text{ADC\_Value}}{4095.0}\right) \times 3.3\text{ V} \times \left(\frac{100\text{ k}\Omega + 22\text{ k}\Omega}{22\text{ k}\Omega}\right) \times \text{CalibrationFactor}$$

---

## 5. Noise Suppression & Failsafe Wiring

1. **Motor Capacitors:** Solder one 100nF (0.1µF) ceramic capacitor across motor terminals to kill brush arcing noise.
2. **Buffer Capacitor:** Place a 470µF to 1000µF 25V electrolytic capacitor right at the motor driver's `VM` and `GND` pins to handle sharp acceleration surges.
3. **Hardware Kill Switch:** Place a master toggle switch directly on the positive terminal wire of the battery before any branch.
