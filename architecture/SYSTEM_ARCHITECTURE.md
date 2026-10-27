# System Architecture — ROBO Race Bot Wireless

## 1. High-Level System Overview

The **ROBO Race Bot Wireless** is an ultra-low-latency, competition-grade 4WD robotics racing platform engineered for rapid reflex response, high torque delivery, and real-time telemetry.

```
       +-----------------------------------------------------------+
       |             HANDHELD REMOTE CONTROLLER (TX)               |
       |  - Standard ESP32 DevKit V1 (240MHz Dual Core)            |
       |  - 4x Tactile Tank-Drive Directional Buttons              |
       |  - 2x Global Speed Trigger Buttons (Base, Preset 1, 2)    |
       |  - 0.96" SSD1306 I2C OLED Display (Live Cockpit Data)     |
       |  - EC11 Rotary Encoder + Pushbutton (On-Remote Settings)  |
       |  - Non-Volatile Flash Memory (Stores All User Settings)   |
       +-----------------------------+-----------------------------+
                                     |
                                     |  ESP-NOW (2.4 GHz Bare-Metal MAC)
                                     |  - 200 Hz Packet Rate (Every 5ms)
                                     |  - Latency < 1.5 - 2.5 ms
                                     |  - Bidirectional Telemetry
                                     v
       +-----------------------------------------------------------+
       |                 ROBOT RECEIVER & CHASSIS (RX)             |
       |  - Espressif ESP32-S3-WROOM-1U-N16 (External U.FL Antenna)|
       |  - Dual-Core FreeRTOS Architecture (Reflex vs Background) |
       |  - 20 kHz Hardware MCPWM Outputs (Zero Motor Coil Whine)  |
       |  - Native USB-C Flashing/Telemetry Port (GPIO 19/20)      |
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

## 2. Power Architecture & Battery Management

The bot uses an internal, permanently mounted **3S 11.1V–12.6V 2200mAh LiPo battery (40C–80C burst)**.

```
                         +-----------------------------+
                         | 3S 2200mAh LiPo (11.1-12.6V)|
                         +--------------+--------------+
                                        | (Internal XT60)
                             [ Master 30A Power Switch ]
                                        |
               +------------------------+------------------------+
               |                                                 |
               v 12V High-Current Rail                           v 12V Logic Tap
      +------------------+                               +------------------+
      | 2x BTS7960       |                               | DC-DC Step-Down  |
      | Motor Drivers    |                               | Buck Converter   |
      | (43A Peak Each)  |                               | (12V -> 3.3V/5V) |
      +--------+---------+                               +--------+---------+
               |                                                  |
               v                                                  v
      [ 4x Johnson Motors ]                              [ ESP32-S3 Module ]
      (Peak ~15A per side)                               (Clean 3.3V Power)
```

### Chassis External Interfaces (No Disassembly Needed):
1. **Master Power Toggle Switch:** Physically isolates the battery from all electronics.
2. **XT60 Male Socket (Flush Panel Mount):** Connects external LiPo balance charger directly to battery terminals.
3. **3S JST-XH 4-Pin Balance Port:** Exposes all 3 individual cell taps for balanced charging.
4. **USB Type-C Port:** Direct link to ESP32-S3 GPIO 19 (`D-`) and GPIO 20 (`D+`) for one-click flashing and serial monitoring.

---

## 3. Real-Time Multiprocessing Model (FreeRTOS)

To ensure **zero control lag**, the ESP32-S3 separates critical motor reflexes from background sensor polling across its two 240MHz Xtensa cores:

```
+-------------------------------------------------------------------------+
|                  CORE 1: THE DEDICATED REFLEX CORE                      |
|                      Priority: 24 (Maximum Real-Time)                   |
|                      Frequency: 200 Hz (Every 5 ms)                     |
+-------------------------------------------------------------------------+
|  1. ESP-NOW Radio Receive (Sub-1ms packet capture)                      |
|  2. 1:1 Instant Tank-Drive Logic (No Slew Rate / No Artificial Ramp)   |
|  3. Hardware 20 kHz MCPWM Output Updates (< 10 microseconds)            |
|  4. Hardware Failsafe Watchdog (Auto-stops if remote dies for > 100ms)  |
+-------------------------------------------------------------------------+

+-------------------------------------------------------------------------+
|                CORE 0: THE BACKGROUND HOUSEKEEPING CORE                 |
|                      Priority: 1 (Lowest Background)                    |
|                      Frequency: Interleaved Periodic Timers             |
+-------------------------------------------------------------------------+
|  1. Motor Current Sensing: Left & Right BTS7960 IS pins (50 Hz)         |
|  2. Battery Voltage Monitoring: 3-Cell Individual ADC reads (10 Hz)     |
|  3. Digital Temperatures: 3x DS18B20 1-Wire non-blocking reads (1 Hz)   |
|  4. Telemetry Transmitter: Bundles telemetry back to Remote (10 Hz)     |
+-------------------------------------------------------------------------+
```

---

## 4. Safety Protection Thresholds

| Sensor Target | Normal Range | Warning Threshold | Emergency Action |
|---|---|---|---|
| **3S LiPo Cell** | $3.70\text{V} - 4.20\text{V}$ | **$< 3.40\text{V}$** | Warning flag on Remote screen |
| **3S LiPo Total** | $11.1\text{V} - 12.6\text{V}$ | **$< 10.20\text{V}$** | Buzzer alert + Emergency Cutoff |
| **Battery Temp** | $25^\circ\text{C} - 42^\circ\text{C}$ | **$> 50^\circ\text{C}$** | 25% Limp Mode to protect cells |
| **BTS7960 Temp** | $30^\circ\text{C} - 65^\circ\text{C}$ | **$> 80^\circ\text{C}$** | Driver thermal warning alert |
| **Motor Stall** | $0\text{A} - 4\text{A}$ running | **$> 12\text{A}$ sustained** | Protects motor brushes from burnout |
| **Radio Link** | Continuous 200 Hz | **$> 100\text{ms}$ no packet** | Immediate 0% Motor Cut (Failsafe) |
