# ROBO Race Bot Wireless — Master Engineering & R&D Compendium

A championship-grade **4WD Invertible Wireless Race Bot** and **Custom Handheld Cockpit Remote** built on **Espressif ESP32-S3**, **BTS7960 43A MOSFET Drivers**, and **Bare-Metal ESP-NOW** for sub-2ms reflex latency, 18.12 kg·cm torque, zero-lag auto-flip remapping, multi-cell telemetry, and multi-layer EMF hardening.

---

## 📑 Table of Contents
1. [Project Executive Summary & R&D Vision](#1-project-executive-summary--rd-vision)
2. [Evolution from Previous Project (ESP32-RoboRace-bot)](#2-evolution-from-previous-project-esp32-roborace-bot)
3. [Component Selection & Technical R&D Analysis](#3-component-selection--technical-rd-analysis)
   - [3.1 Motors: Johnson 12V 600RPM (RKI-1143)](#31-motors-johnson-12v-600rpm-rki-1143)
   - [3.2 Motor Drivers: Dual BTS7960 43A H-Bridges](#32-motor-drivers-dual-bts7960-43a-h-bridges)
   - [3.3 MCU: Espressif ESP32-S3-WROOM-1U-N16](#33-mcu-espressif-esp32-s3-wroom-1u-n16)
   - [3.4 Battery: 3S 2200mAh LiPo (40C–80C) Power Math](#34-battery-3s-2200mah-lipo-40c80c-power-math)
4. [Failure Mode & Critical Risk Engineering (FMEA)](#4-failure-mode--critical-risk-engineering-fmea)
   - [4.1 Motor Back-EMF & 5-Layer Noise Shield](#41-motor-back-emf--5-layer-noise-shield)
   - [4.2 Shoot-Through Protection & Break-Before-Make](#42-shoot-through-protection--break-before-make)
   - [4.3 I2C Bus Lockup Protection (MPU-6050)](#43-i2c-bus-lockup-protection-mpu-6050)
   - [4.4 False Low-Voltage Filtering (EMA Filter)](#44-false-low-voltage-filtering-ema-filter)
5. [Auto-Invert Flip Remapping Kinematics](#5-auto-invert-flip-remapping-kinematics)
6. [FreeRTOS Dual-Core Architecture: "Core 1 is Sacred"](#6-freertos-dual-core-architecture-core-1-is-sacred)
7. [Handheld Remote Controller Architecture](#7-handheld-remote-controller-architecture)
   - [7.1 4-Button Tank Drive Reflex Matrix](#71-4-button-tank-drive-reflex-matrix)
   - [7.2 3-Tier Global Speed Overrides](#72-3-tier-global-speed-overrides)
   - [7.3 OLED Telemetry & EC11 Rotary Navigation](#73-oled-telemetry--ec11-rotary-navigation)
8. [Chassis & External Port Engineering](#8-chassis--external-port-engineering)
   - [8.1 GX16 6-Pin Aviation Fast-Charge & Balance Port](#81-gx16-6-pin-aviation-fast-charge--balance-port)
   - [8.2 Panel-Mount USB-C with 5.1kΩ CC Resistors](#82-panel-mount-usb-c-with-51k-cc-resistors)
9. [Charger Benchmarking: ISDT PD60 vs B3AC](#9-charger-benchmarking-isdt-pd60-vs-b3ac)
10. [Hardware Pinout Reference](#10-hardware-pinout-reference)
11. [Future Roadmap (Phase 2 Distributed Architecture)](#11-future-roadmap-phase-2-distributed-architecture)

---

## 1. Project Executive Summary & R&D Vision

Competitive robotics racing demands three non-negotiable performance pillars:
1. **Zero-Latency Control Reflex (< 2ms):** Standard RC radios (50 Hz / 20ms) and Wi-Fi/Bluetooth stacks (30–80ms) create a sluggish, disconnected driving experience. By utilizing bare-metal ESP-NOW at 200 Hz with dual-core FreeRTOS task pinning, this bot achieves instantaneous reflex engagement.
2. **Instant Acceleration & Saturated Power:** Old bipolar H-bridges (L298N) drop 2.5V–3.5V as waste heat. Dual BTS7960 MOSFET bridges ($16\text{ m}\Omega$ on-resistance) deliver uncompromised 12V battery power to 4 parallel Johnson motors, generating **18.12 kg·cm combined stall torque**.
3. **Unbreakable Reliability & Crash Resilience:** Invertible tank chassis capability allows driving upside-down. With onboard MPU-6050 6-axis sensing, controls **auto-remap in under 5 nanoseconds** so the driver never needs to physically invert the remote during crashes.

```
       +-----------------------------------------------------------+
       |             HANDHELD REMOTE CONTROLLER (TX)               |
       |  - Standard ESP32 DevKit V1 (240MHz Dual Core)            |
       |  - 4x Tactile Tank-Drive Directional Buttons              |
       |  - 2x Global Speed Trigger Buttons (Base, Preset 1, 2)    |
       |  - 0.96" SSD1306 I2C OLED Display (Live Cockpit Data)     |
       |  - EC11 Rotary Encoder + Pushbutton (On-Remote Settings)  |
       |  - Coin Haptic Vibration Motor (Speed Shift & Alerts)     |
       |  - Non-Volatile Flash Memory (Stores All User Settings)   |
       +-----------------------------+-----------------------------+
                                     |
                                     |  ESP-NOW (2.4 GHz Bare-Metal MAC)
                                     |  - 200 Hz Packet Rate (Every 5ms)
                                     |  - Latency < 1.5 - 2.5 ms
                                     |  - Bidirectional Telemetry & Failsafe
                                     v
       +-----------------------------------------------------------+
       |                 ROBOT RECEIVER & CHASSIS (RX)             |
       |  - Espressif ESP32-S3-WROOM-1U-N16 (External U.FL Antenna)|
       |  - Dual-Core FreeRTOS Architecture (Reflex vs Background) |
       |  - 20 kHz Hardware MCPWM Outputs (Zero Motor Coil Whine)  |
       |  - Native USB-C Flashing/Telemetry Port (GPIO 19/20)      |
       |  - MPU-6050 6-Axis IMU (Auto-Invert Flip Remapping)       |
       |  - 5-Layer Motor EMF & Inductive Spike Protection Shield  |
       +-----------------------------+-----------------------------+
                                     |
         +---------------------------+---------------------------+
         |                                                       |
         v Left Drive Channel                                    v Right Drive Channel
+-------------------+                                   +-------------------+
| 1x BTS7960 Driver |                                   | 1x BTS7960 Driver |
| 43A Peak H-Bridge |                                   | 43A Peak H-Bridge |
+---------+---------+                                   +---------+---------+
          |                                                       |
     (In Parallel)                                           (In Parallel)
          |                                                       |
    +-----+-----+                                           +-----+-----+
    |           |                                           |           |
    v           v                                           v           v
[Motor 1]   [Motor 3]                                   [Motor 2]   [Motor 4]
Left-Front  Left-Rear                                   Right-Front Right-Rear
(12V Johnson 600RPM)                                    (12V Johnson 600RPM)
```

---

## 2. Evolution from Previous Project (`ESP32-RoboRace-bot`)

This repository builds upon the foundational concepts explored in [Bhavesh6/ESP32-RoboRace-bot](https://github.com/Bhavesh6/ESP32-RoboRace-bot) while fundamentally re-engineering every subsystem for competition reliability:

| Subsystem | Previous Build (`ESP32-RoboRace-bot`) | Current Build (`robo-race-bot-wireless`) | Competitive Upgrade & Justification |
|---|---|---|---|
| **Robot MCU** | Standard ESP32 (PCB antenna) | **ESP32-S3-WROOM-1U-N16** | External U.FL antenna avoids metal/carbon RF shielding; Native USB-C on GPIO 19/20 eliminates external programmer boards. |
| **PWM Generation** | Software PWM (~1 kHz) | **20 kHz Hardware MCPWM** | Ultrasonic frequency eliminates audible motor whine and achieves instant magnetic coil saturation. |
| **System Scheduling** | Single Arduino `loop()` | **FreeRTOS Dual-Core Asymmetric** | Core 1 pinned exclusively to 200 Hz motor reflexes; Core 0 handles sensors with zero latency penalty on driving. |
| **Battery Monitoring** | External INA226 module (Pack only) | **3S Individual Balance Dividers** | Monitors Cell 1, 2, and 3 individually via 3S balance port; catches sagging or unbalanced cells before failure. |
| **Current Sensing** | INA226 + 25A physical shunt | **Built-in BTS7960 `IS` Lines** | Reads Left and Right motor current independently directly from driver pins with no bulky external shunt. |
| **Orientation Safety** | Manual driver mental inversion | **MPU-6050 Auto-Flip Remap** | Detects inverted chassis ($A_z < -0.35g$) and swaps controls in $< 5\text{ ns}$ (1 CPU clock cycle). |
| **Thermal Protection** | None | **3x DS18B20 1-Wire Digital** | Real-time monitoring of LiPo battery and both driver heatsinks with automatic 60°C thermal limp mode. |
| **Remote Controls** | Dual analog thumb joysticks | **4 Direct Tank Buttons + Dual Triggers** | Eliminates mechanical joystick travel and deadzone latency (< 1ms microswitch click vs 15ms joystick swing). |

---

## 3. Component Selection & Technical R&D Analysis

### 3.1 Motors: Johnson 12V 600RPM (`RKI-1143`)
* **Base Motor Speed:** 18,000 RPM internal armature
* **Gear Ratio:** 1 : 30 reduction with all-metal spur gears
* **Output Speed:** 600 RPM @ nominal 12V DC
* **Shaft Type:** $\varnothing 6\text{ mm}$ off-centered side shaft with internal M3 tapped mounting hole
* **Rated Torque:** 2.0 kg·cm (0.196 N·m)
* **Stall Torque:** 4.53 kg·cm (0.444 N·m) per motor
* **No-Load Current ($I_0$):** 0.8 A
* **Stall / Peak Current ($I_{stall}$):** 7.5 A per motor
* **4WD Parallel Drivetrain Totals:**
  * 2 motors wired in parallel per side $\rightarrow$ **15.0 A stall per side**
  * Total robot stall current (all 4 wheels locked) $\rightarrow$ **30.0 A total**
  * Total robot stall torque $\rightarrow$ **18.12 kg·cm combined**

### 3.2 Motor Drivers: Dual BTS7960 43A H-Bridges
* **Peak Current Rating:** 43 A per module
* **Continuous Current:** 15 A – 20 A with integrated aluminum heatsink
* **On-Resistance ($R_{DS(on)}$):** $16\text{ m}\Omega$ typical path resistance
* **Voltage Drop Calculation:**
  $$\Delta V = I \times R_{DS(on)} = 7.5\text{ A} \times 0.016\,\Omega = 0.12\text{ V}$$
  *(Compared to the L298N's $2.5\text{V} - 3.2\text{V}$ drop, the BTS7960 delivers 99% of the battery's voltage directly to the Johnson motors).*
* **Built-in Protections:** Over-temperature latch, over-voltage lockout, under-voltage shutdown, and current limitation.

### 3.3 MCU: Espressif ESP32-S3-WROOM-1U-N16
* **CPU:** Xtensa dual-core 32-bit LX7 @ up to 240 MHz with vector instructions
* **Flash:** 16 MB Quad SPI Flash (`N16`)
* **SRAM:** 512 KB on-chip SRAM + 384 KB ROM + 16 KB RTC SRAM
* **Antenna (`1U`):** External antenna via I-PEX / U.FL (MHF I) connector. Allows mounting an external 3dBi omni antenna outside metal/carbon fiber chassis.
* **USB Subsystem:** Native USB 2.0 Full-Speed OTG & USB Serial/JTAG controller directly on GPIO 19 (`D-`) and GPIO 20 (`D+`).

### 3.4 Battery: 3S 2200mAh LiPo (40C–80C) Power Math
* **Nominal Voltage:** 11.1 V (3.7 V / cell)
* **Full Charge Voltage:** 12.6 V (4.2 V / cell)
* **Cutoff Voltage:** 10.2 V (3.4 V / cell critical threshold)
* **Continuous Discharge (40C–60C):**
  $$I_{continuous} = 2.2\text{ Ah} \times 40\text{C} = \mathbf{88\text{ A}}$$
  $$I_{continuous(60C)} = 2.2\text{ Ah} \times 60\text{C} = \mathbf{132\text{ A}}$$
* **Burst Discharge (80C):**
  $$I_{burst} = 2.2\text{ Ah} \times 80\text{C} = \mathbf{176\text{ A Peak!}}$$
* **5-Minute Race Energy Consumption:**
  At an average continuous racing draw of 12A–15A across a 5-minute round:
  $$\text{Capacity Used} = 15\text{ A} \times \left(\frac{5}{60}\text{ h}\right) = 1.25\text{ Ah} = \mathbf{1250\text{ mAh}}$$
  The pack finishes the race with **~43% remaining capacity**, ensuring the battery never sags into the low-voltage tail and maintains maximum top speed (600 RPM) from start to finish.

---

## 4. Failure Mode & Critical Risk Engineering (FMEA)

In competitive racing, failures occur at interface boundaries (noise, thermal runaway, and bus deadlocks). The following engineering mitigations are implemented:

### 4.1 Motor Back-EMF & 5-Layer Noise Shield
Brushed motors generate brush arcing and high-voltage inductive flyback ($V = -L \frac{di}{dt}$):

```
           +-----------------------------+
           |       JOHNSON MOTOR         |
           |                             |
      (+) -+- [ 100nF Cap ] -+           |
           |                 |           |
           +-- [ 100nF ] ----+-------- (Metal Casing / Earth)
           |                 |           |
      (-) -+- [ 100nF Cap ] -+           |
           +-----------------------------+
```

1. **Motor Snubbers:** Three 100nF 50V ceramic caps soldered directly to each motor absorb high-frequency commutator brush sparks at the source.
2. **Power Rail Isolation:** A Schottky diode (1N5819 / SS34) and a 1000 µF 25V Low-ESR capacitor isolate the logic buck converter from high-current motor transients:
   ```
   12V LiPo ----+----------------------------------------> BTS7960 Drivers
                |
                +----[ 1N5819 Schottky Diode ]----+------> Buck Converter In (3.3V)
                                                  |
                                            [ 1000 µF ]
                                                  |
                                                 GND
   ```
3. **Star Grounding:** Heavy motor ground currents return directly to the battery negative terminal; logic ground connects via a dedicated clean wire.
4. **BTS7960 Active Freewheeling:** Internal body diodes recirculate inductive surges within the driver H-bridge.
5. **External U.FL Antenna:** Physical separation between the 2.4 GHz antenna and internal motor brushes prevents RF interference.

### 4.2 Shoot-Through Protection & Break-Before-Make
Instant switching between forward and reverse can cause high-side and low-side MOSFETs to conduct simultaneously, producing a destructive short-circuit across the LiPo. 
The firmware enforces **Break-Before-Make** with a 3 microsecond dead-time delay:
```cpp
// 1. Force both PWM lines to 0 (Turn all gates OFF)
ledcWrite(CH_LEFT_LPWM, 0);
ledcWrite(CH_LEFT_RPWM, 0);
// 2. Hardware dead-time delay
esp_rom_delay_us(3);
// 3. Apply new direction safely
ledcWrite(CH_LEFT_RPWM, target_speed);
```

### 4.3 I2C Bus Lockup Protection (MPU-6050)
Standard Arduino `Wire.h` hangs the CPU indefinitely if noise corrupts the I2C acknowledge bit. 
* We enforce a **2 millisecond hardware timeout**: `Wire.setTimeOut(2)`.
* If the MPU-6050 drops off the bus during a collision, the bot logs an error and **continues driving at 100% speed** in normal orientation.

### 4.4 False Low-Voltage Filtering (EMA Filter)
Sudden acceleration causes temporary battery sag. To prevent false emergency cutoffs, the ADC reads are filtered through a 10-sample **Exponential Moving Average (EMA)**:
$$V_{\text{filtered}} = 0.90 \times V_{\text{previous}} + 0.10 \times V_{\text{raw}}$$
A low-voltage cutoff triggers only if voltage stays below 10.2V for **more than 1.5 continuous seconds**.

---

## 5. Auto-Invert Flip Remapping Kinematics

When an invertible tank robot flips upside down:
1. **Left and Right swap positions** relative to the driver.
2. **Tire ground contact reverses** (what was the top of the wheel now touches the ground). Spinning the motor "forward" pushes the robot **backward**.

```
             NORMAL (Right-Side Up)                     FLIPPED (Upside Down)
             
                 +-----------+                              +-----------+
                 |   FRONT   |                              |   REAR    |  <-- Now facing you
  [Left Wheel]   |           | [Right Wheel] [Right Wheel]  |           | [Left Wheel]
  (Motor A)      |           | (Motor B)     (Motor B)      |           | (Motor A)
                 +-----------+                              +-----------+
                 
  * Ground Contact: Bottom of tire           * Ground Contact: What was TOP of tire!
  * Forward Motor Rotation = Goes FORWARD    * Forward Motor Rotation = Goes BACKWARD!
```

### The 1-Cycle Hardware Correction Formula
To maintain natural driver controls without rotating the remote:
```cpp
if (g_isFlipped) {
    // 1. Swap Left <-> Right channels
    // 2. Negate polarity (-) to convert Reverse tire spin into Forward travel
    actualLeft  = -desiredRight;
    actualRight = -desiredLeft;
} else {
    actualLeft  = desiredLeft;
    actualRight = desiredRight;
}
```
* **Latency:** Executes in **1 CPU clock cycle (< 5 nanoseconds)**.
* **Anti-Bounce Filter:** Orientation change requires $A_z < -0.35g$ for $> 50\text{ ms}$ to prevent accidental flips during jumps.

---

## 6. FreeRTOS Dual-Core Architecture: "Core 1 is Sacred"

To eliminate control lag, tasks are strictly segregated across the two 240MHz CPU cores:

```
========================= CORE 1 : THE "REFLEX" CORE =========================
Priority: HIGHEST (Priority 24) | Frequency: 200 Hz (Every 5 ms)
Responsibilities:
  - ESP-NOW Radio Receive (Sub-1ms packet capture)
  - Auto-Invert Flip Remapping (1 CPU cycle execution)
  - 1:1 Instant Tank-Drive Math (No artificial slew rates)
  - 20 kHz Hardware MCPWM Updates (< 10 microseconds)
  - 100 ms Failsafe Watchdog (Auto-brakes if radio drops)
==> ZERO BLOCKING. Stick moves -> Motors turn in under 2 milliseconds! <==

======================= CORE 0 : THE "HOUSEKEEPING" CORE =====================
Priority: LOW / BACKGROUND (Priority 1) | Frequency: Interleaved Timers
Responsibilities:
  - MPU-6050 Orientation Read (50 Hz / 20 ms with 2ms timeout)
  - 3S Balance Port ADC Monitoring (20 Hz / 50 ms with EMA filter)
  - BTS7960 Current Sense ADC Reads (20 Hz / 50 ms)
  - DS18B20 1-Wire Digital Temps (1 Hz / 1000 ms - Non-blocking)
  - Bidirectional Telemetry Transmission to Remote (10 Hz / 100 ms)
==> Completely isolated. Even if a sensor stalls, Core 1 NEVER waits! <==
```

---

## 7. Handheld Remote Controller Architecture

### 7.1 4-Button Tank Drive Reflex Matrix
Four physical tactile switches provide zero deadband and microsecond response:

| Upper Left (GPIO 13) | Lower Left (GPIO 12) | Upper Right (GPIO 14) | Lower Right (GPIO 27) | Robot Motion | Left Motor | Right Motor |
|:---:|:---:|:---:|:---:|---|:---:|:---:|
| **PRESSED** | — | **PRESSED** | — | **Straight Forward** | FORWARD | FORWARD |
| — | **PRESSED** | — | **PRESSED** | **Straight Reverse** | REVERSE | REVERSE |
| **PRESSED** | — | — | **PRESSED** | **360° Clockwise Spin** | FORWARD | REVERSE |
| — | **PRESSED** | **PRESSED** | — | **360° Counter-Clockwise Spin** | REVERSE | FORWARD |
| **PRESSED** | — | — | — | **Sweeping Turn Right** | FORWARD | BRAKE |
| — | — | **PRESSED** | — | **Sweeping Turn Left** | BRAKE | FORWARD |
| — | — | — | — | **Active Electronic Brake** | BRAKE | BRAKE |

### 7.2 3-Tier Global Speed Overrides
Two index-finger trigger buttons act as instantaneous speed shifts:
* **No Triggers Held:** Movement runs at **Base Speed** (default: 400 RPM / 66% PWM).
* **Hold Trigger 1 (GPIO 32):** Movement runs at **Preset 1 Speed** (default: 500 RPM / 83% PWM).
* **Hold Trigger 2 (GPIO 33):** Movement runs at **Preset 2 Speed** (default: 600 RPM / 100% Full Punch).

### 7.3 OLED Telemetry & EC11 Rotary Navigation
* **Page 0 (Live Dashboard):** Total Pack Voltage, individual cell voltages ($C_1, C_2, C_3$), motor currents ($I_L, I_R$), temperatures (Battery, Left Driver, Right Driver), RPM mode, `[FLIP]` state indicator, and radio link status.
* **Page 1 (Speed Tuning):** Rotate encoder to adjust RPMs in steps of 25 (200 to 600 RPM). Click to save to **NVS Flash Memory**.
* **Haptic Feedback:** Coin vibration motor pulses on speed trigger engagement and emits continuous vibration alerts on low voltage or high temperatures.

---

## 8. Chassis & External Port Engineering

Because the 3S LiPo is permanently sealed inside the robot chassis, external service ports are mounted on the rear panel:

```
 CHASSIS REAR PANEL:
+-------------------------------------------------------------------------+
|  [ MASTER SWITCH ]       [ GX16 6-PIN PORT ]       [ USB-C DATA PORT ]  |
|   (30A Isolation)       (Fast-Charge & Balance)     (Native Flash/Logs) |
+-------------------------------------------------------------------------+
```

### 8.1 GX16 6-Pin Aviation Fast-Charge & Balance Port
* **Pin 1:** Battery Positive (`BAT +`) — 5A rated
* **Pin 2:** Battery Negative (`BAT −`) — 5A rated
* **Pin 3:** Cell 1 Balance Tap (3.7V – 4.2V)
* **Pin 4:** Cell 2 Balance Tap (7.4V – 8.4V)
* **Pin 5:** Cell 3 Balance Tap (11.1V – 12.6V)
* **Pin 6:** Balance Ground Return

### 8.2 Panel-Mount USB-C with 5.1kΩ CC Resistors
* **Red:** `VBUS` (5V with Schottky diode protection)
* **Black:** `GND`
* **White:** `D-` $\rightarrow$ ESP32-S3 GPIO 19
* **Blue:** `D+` $\rightarrow$ ESP32-S3 GPIO 20
* **Green & Yellow:** `CC1` & `CC2` connected to **GND via two $5.1\text{ k}\Omega$ resistors** (enables modern USB-C to USB-C laptop charging cables).

---

## 9. Charger Benchmarking: ISDT PD60 vs B3AC

| Specification | B3AC Compact Charger | ISDT PD60 Smart Charger (Selected) |
|---|---|---|
| **Max Charge Current** | 0.8 A (800 mA) | **6.0 A / 60 W** |
| **Charge Time (3S 2200mAh)** | **~2.5 to 3.0 Hours** ❌ | **25 to 30 Minutes (at 2C / 4.4A)** ✅ |
| **Power Source** | AC 100-240V Wall socket only | **USB Type-C PD / QC** (Laptop charger / Power Bank) |
| **Telemetry & Display** | 3 status LEDs (Dumb) | Audio buzzer, precise voltage sensing, auto cell detect |
| **Competition Suitability**| Unusable for fast turnaround | **Championship Grade** |

---

## 10. Hardware Pinout Reference

### Robot Receiver (ESP32-S3-WROOM-1U)
| Function | GPIO | Mode | Description |
|---|---|---|---|
| **Left RPWM** | `GPIO 17` | Output | Left Motor Forward (20 kHz MCPWM) |
| **Left LPWM** | `GPIO 18` | Output | Left Motor Reverse (20 kHz MCPWM) |
| **Left EN** | `GPIO 15` | Output | Left Driver Enable |
| **Right RPWM** | `GPIO 38` | Output | Right Motor Forward (20 kHz MCPWM) |
| **Right LPWM** | `GPIO 39` | Output | Right Motor Reverse (20 kHz MCPWM) |
| **Right EN** | `GPIO 40` | Output | Right Driver Enable |
| **MPU-6050 SDA**| `GPIO 1` | I2C | Orientation Sensor Data |
| **MPU-6050 SCL**| `GPIO 2` | I2C | Orientation Sensor Clock |
| **Cell 1 ADC** | `GPIO 4` | Analog (ADC1) | 0–4.2V via 10k/10k divider |
| **Cell 2 ADC** | `GPIO 5` | Analog (ADC1) | 0–8.4V via 30k/10k divider |
| **Cell 3 ADC** | `GPIO 6` | Analog (ADC1) | 0–12.6V via 50k/10k divider |
| **Left Current**| `GPIO 7` | Analog (ADC1) | Left BTS7960 `IS` Current Sense |
| **Right Current**| `GPIO 8` | Analog (ADC1) | Right BTS7960 `IS` Current Sense |
| **DS18B20 Bus** | `GPIO 14` | Digital | 3x Temperatures (Battery + 2 Drivers) |
| **Native USB D-**| `GPIO 19` | USB | Native Serial/JTAG Data - |
| **Native USB D+**| `GPIO 20` | USB | Native Serial/JTAG Data + |

### Handheld Remote (Standard ESP32 DevKit V1)
| Function | GPIO | Mode | Description |
|---|---|---|---|
| **Upper Left Button** | `GPIO 13` | Input (`PULLUP`)| Tank Left Forward |
| **Lower Left Button** | `GPIO 12` | Input (`PULLUP`)| Tank Left Reverse |
| **Upper Right Button**| `GPIO 14` | Input (`PULLUP`)| Tank Right Forward |
| **Lower Right Button**| `GPIO 27` | Input (`PULLUP`)| Tank Right Reverse |
| **Trigger 1 Button** | `GPIO 32` | Input (`PULLUP`)| Speed Preset 1 Override (500 RPM) |
| **Trigger 2 Button** | `GPIO 33` | Input (`PULLUP`)| Speed Preset 2 Override (600 RPM) |
| **Haptic Motor** | `GPIO 15` | Output | Coin vibration motor (NPN driven) |
| **Encoder CLK** | `GPIO 25` | Input (`PULLUP`)| EC11 Rotary Phase A |
| **Encoder DT** | `GPIO 26` | Input (`PULLUP`)| EC11 Rotary Phase B |
| **Encoder SW** | `GPIO 4` | Input (`PULLUP`)| EC11 Rotary Click Button |
| **OLED SDA** | `GPIO 21` | I2C | SSD1306 Display Data |
| **OLED SCL** | `GPIO 22` | I2C | SSD1306 Display Clock |

---

## 11. Future Roadmap (Phase 2 Distributed Architecture)

If real-world track testing ever reveals sensor noise or demands complete physical hardware isolation, the architecture is designed to support **Phase 2 Modular Separation**:
* **ESP32-S3:** Remains the pure, dedicated Motor & Radio reflex driver.
* **ESP32-C3 SuperMini:** Added as a co-processor to handle all sensor polling (Battery ADC, DS18B20, MPU-6050) and transmit telemetry directly to the remote, connected to the S3 via a single digital `FLIP_STATE` hardware pin.

---

## 📄 License & Attribution
Engineered by Ghost. Open-source under the MIT License for competitive robotics enthusiasts.
