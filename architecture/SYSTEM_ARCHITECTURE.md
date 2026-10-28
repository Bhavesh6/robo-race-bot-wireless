# System Architecture — ROBO Race Bot Wireless

## 1. High-Level System Overview

The **ROBO Race Bot Wireless** is an ultra-low-latency, competition-grade 4WD robotics racing platform engineered for rapid reflex response, high torque delivery, real-time telemetry, and inverted-chassis race resilience.

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

## 2. Power Architecture & EMF Protection Shield

The bot is powered by an internal, permanently mounted **3S 11.1V–12.6V 2200mAh LiPo battery (40C–80C burst)**.

```
                         +-----------------------------+
                         | 3S 2200mAh LiPo (11.1-12.6V)|
                         +--------------+--------------+
                                        | (Internal XT60)
                             [ Master 30A Power Switch ]
                                        |
               +------------------------+------------------------+
               | (High-Current 12V Rail)                         | (Logic Power Tap)
               v                                                 v
      +------------------+                             [ 1N5819 Schottky Diode ]
      | 2x BTS7960       |                                       |
      | Motor Drivers    |                                 [ 1000 µF 25V ]
      | (43A Peak Each)  |                                 Buffer Capacitor
      +--------+---------+                                       |
               |                                                 v
               v                                       +------------------+
      [ 4x Johnson Motors ]                            | DC-DC Step-Down  |
      (Peak ~15A per side)                             | Buck Converter   |
      - 3x 100nF Snubbers/motor                        | (12V -> 3.3V)    |
                                                       +--------+---------+
                                                                |
                                                                v
                                                       [ ESP32-S3 Module ]
```

### The 5-Layer Motor EMF & Noise Shield:
1. **Motor Snubbers:** Three 100nF 50V ceramic capacitors soldered directly across every motor's terminals (+ to -, + to can, - to can) suppress brush arcing at the physical source.
2. **Power Isolation Diode:** A Schottky diode (1N5819 / SS34) prevents the motors from pulling power out of the logic buffer capacitor during voltage sags.
3. **1000 µF Low-ESR Buffer Capacitor:** Absorbs inductive back-EMF spikes when motors reverse suddenly, ensuring zero brownout resets on the ESP32-S3.
4. **Star Grounding:** High-current motor ground returns directly to the battery XT60; the ESP32 logic ground is wired independently to the central ground point to eliminate ground bounce.
5. **External U.FL Antenna:** Mounted on the outside chassis wall, isolated from internal electrical noise.

---

## 3. Auto-Invert Flip Remapping (Zero-Lag Orientation Adaptation)

In competitive racing, invertible tank bots often get flipped upside down during crashes or collisions. 

```
NORMAL ORIENTATION (Az > +0.3g)                FLIPPED ORIENTATION (Az < -0.3g)
      +----------+                                    +----------+
  [L] |  FRONT   | [R]                            [R] |  BOTTOM  | [L]
      +----------+                                    +----------+
 Left = Left Motor, Fwd = Fwd                    Left = -Right Motor, Fwd = -Rev Motor
```

* **Core 0 Sensor Polling:** Reads the MPU-6050 accelerometer Z-axis ($A_z$) at 50 Hz. If $A_z < -0.3g$ for $> 50\text{ms}$ (anti-bounce filter), it sets `is_flipped = true`.
* **Core 1 Zero-Lag Execution:** Remapping takes **less than 5 nanoseconds (1 CPU cycle)**:
  ```cpp
  if (is_flipped) {
      actual_left  = -desired_right;
      actual_right = -desired_left;
  }
  ```
* **Driver Benefit:** The driver does **NOT** need to rotate the remote or mentally reverse controls. You press forward, the bot drives forward; you press left, the bot turns left!

---

## 4. Real-Time Multiprocessing Model (FreeRTOS)

```
+-------------------------------------------------------------------------+
|                  CORE 1: THE DEDICATED REFLEX CORE                      |
|                      Priority: 24 (Maximum Real-Time)                   |
|                      Frequency: 200 Hz (Every 5 ms)                     |
+-------------------------------------------------------------------------+
|  1. ESP-NOW Radio Receive (Sub-1ms packet capture)                      |
|  2. Auto-Invert Flip Remapping (1 CPU cycle execution)                  |
|  3. 1:1 Instant Tank-Drive Logic (No Slew Rate / No Artificial Ramp)   |
|  4. Hardware 20 kHz MCPWM Output Updates (< 10 microseconds)            |
|  5. Hardware Failsafe Watchdog (Auto-stops if remote dies for > 100ms)  |
+-------------------------------------------------------------------------+

+-------------------------------------------------------------------------+
|                CORE 0: THE BACKGROUND HOUSEKEEPING CORE                 |
|                      Priority: 1 (Lowest Background)                    |
|                      Frequency: Interleaved Periodic Timers             |
+-------------------------------------------------------------------------+
|  1. MPU-6050 Orientation Read: Detects roll/flip state (50 Hz)          |
|  2. Motor Current Sensing: Left & Right BTS7960 IS pins (50 Hz)         |
|  3. Battery Voltage Monitoring: 3-Cell Individual ADC reads (10 Hz)     |
|  4. Digital Temperatures: 3x DS18B20 1-Wire non-blocking reads (1 Hz)   |
|  5. Telemetry Transmitter: Bundles telemetry back to Remote (10 Hz)     |
+-------------------------------------------------------------------------+
```
