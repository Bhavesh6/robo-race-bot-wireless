# Hardware Pinout & Wiring Specifications

This document defines the complete electrical pin mappings, voltage divider calculations, sensor circuits, and noise protection shields for both the **Bot Receiver (ESP32-S3)** and the **Handheld Remote (Standard ESP32)**.

---

# Part 1: Robot Receiver (ESP32-S3-WROOM-1U-N16)

The bot uses the compact SMD module with an external **U.FL antenna** and **native USB-C** debugging/flashing.

## 1.1 Complete GPIO Pin Assignment Table

| Pin No. | Label | Connection Target | Circuit / Peripheral | Notes |
|---|---|---|---|---|
| **Pin 1** | `GND` | Power GND Bus | Common Ground Reference | Star-ground point at battery XT60 |
| **Pin 2** | `3V3` | Buck Converter Output | Clean 3.3V Logic Supply ($\ge 500\text{mA}$) | Decoupled with 10µF + 100nF caps |
| **Pin 3** | `EN` | Reset Line | 10kΩ Pull-Up to 3.3V + 1µF Cap to GND | Power-on clean reset circuit |
| **Pin 4** | `GPIO 4` | Cell 1 Voltage Divider | `ADC1_CH3` Analog Input | 10kΩ / 10kΩ divider (0–4.2V $\rightarrow$ 0–2.1V) |
| **Pin 5** | `GPIO 5` | Cell 2 Voltage Divider | `ADC1_CH4` Analog Input | 30kΩ / 10kΩ divider (0–8.4V $\rightarrow$ 0–2.1V) |
| **Pin 6** | `GPIO 6` | Cell 3 Voltage Divider | `ADC1_CH5` Analog Input | 50kΩ / 10kΩ divider (0–12.6V $\rightarrow$ 0–2.1V) |
| **Pin 7** | `GPIO 7` | Left Driver `L_IS` | `ADC1_CH6` Analog Input | Current sense ($1\text{ k}\Omega$ shunt to GND) |
| **Pin 8** | `GPIO 8` | Right Driver `R_IS` | `ADC1_CH7` Analog Input | Current sense ($1\text{ k}\Omega$ shunt to GND) |
| **Pin 39**| `GPIO 1` | MPU-6050 `SDA` | I2C Data Line | 6-Axis IMU (Flip Auto-Remap) |
| **Pin 38**| `GPIO 2` | MPU-6050 `SCL` | I2C Clock Line | 6-Axis IMU (Flip Auto-Remap) |
| **Pin 13**| `GPIO 19` | USB Type-C `D-` | Native USB 2.0 Full-Speed Data - | Built-in USB Serial/JTAG controller |
| **Pin 14**| `GPIO 20` | USB Type-C `D+` | Native USB 2.0 Full-Speed Data + | Built-in USB Serial/JTAG controller |
| **Pin 22**| `GPIO 14` | DS18B20 1-Wire Data | 3x Digital Temperature Sensors | $4.7\text{ k}\Omega$ pull-up to 3.3V |
| **Pin 27**| `GPIO 0` | Boot Strapping Pin | 10kΩ Pull-Up to 3.3V | Ensures normal SPI flash boot |
| **Pin 10**| `GPIO 17` | Left BTS7960 `RPWM` | Hardware MCPWM Channel 0 (20 kHz) | Left Motor Forward Speed |
| **Pin 11**| `GPIO 18` | Left BTS7960 `LPWM` | Hardware MCPWM Channel 1 (20 kHz) | Left Motor Reverse Speed |
| **Pin 8** | `GPIO 15` | Left BTS7960 `EN` | Digital Output (High = Active) | Bridges `R_EN` and `L_EN` together |
| **Pin 31**| `GPIO 38` | Right BTS7960 `RPWM` | Hardware MCPWM Channel 2 (20 kHz) | Right Motor Forward Speed |
| **Pin 32**| `GPIO 39` | Right BTS7960 `LPWM` | Hardware MCPWM Channel 3 (20 kHz) | Right Motor Reverse Speed |
| **Pin 33**| `GPIO 40` | Right BTS7960 `EN` | Digital Output (High = Active) | Bridges `R_EN` and `L_EN` together |
| **Pin 40**| `GND` | Power GND Bus | Additional Ground Pad | Solder directly to GND plane |
| **Pin 41**| `EPAD` | Thermal Ground | Exposed Bottom Ground Pad | Heatsink dissipation |

---

## 1.2 Chassis Ports

### A. GX16 6-Pin Aviation Port (Battery Fast-Charge & Balance)
```
               GX16 6-PIN CONNECTOR (REAR CHASSIS)
                       +---------------+
                       |    (1)   (2)  |   <-- Pin 1: BAT + (Charge) | Pin 2: BAT - (GND)
                       |  (6)       (3)|   <-- Pin 6: Bal Tap 0 (GND)| Pin 3: Bal Tap 1 (4.2V)
                       |    (5)   (4)  |   <-- Pin 5: Bal Tap 3 (12V)| Pin 4: Bal Tap 2 (8.4V)
                       +---------------+
```
* **Pin 1:** Battery Positive (`BAT +`) — 5A rated
* **Pin 2:** Battery Negative (`BAT −`) — 5A rated
* **Pin 3:** Cell 1 Balance Tap (3.7V – 4.2V)
* **Pin 4:** Cell 2 Balance Tap (7.4V – 8.4V)
* **Pin 5:** Cell 3 Balance Tap (11.1V – 12.6V)
* **Pin 6:** Balance Ground Return

### B. USB-C 6-Pin Panel Mount (JST-PH 2.0mm)
* **Red:** `VBUS` (5V with Schottky diode isolation)
* **Black:** `GND` (Common Ground)
* **White:** `D-` $\rightarrow$ **ESP32-S3 GPIO 19**
* **Blue/Green:** `D+` $\rightarrow$ **ESP32-S3 GPIO 20**
* **Green:** `CC1` $\rightarrow$ **$5.1\text{ k}\Omega$ resistor to GND** (Enables Type-C fast cables)
* **Yellow:** `CC2` $\rightarrow$ **$5.1\text{ k}\Omega$ resistor to GND** (Enables Type-C fast cables)

---

## 1.3 Motor Back-EMF & Noise Shield Circuits

### A. Motor Terminal Snubbers (3x 100nF Ceramic Caps per Motor)
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

### B. Power Rail Isolation & Buffer Filter
```
 3S LiPo 12V ----+----------------------------------------> BTS7960 Drivers (Motors)
                 |
                 +----[ 1N5819 Schottky Diode ]----+------> Buck Converter Input
                                                   |        (Outputs 3.3V to ESP32-S3)
                                             [ 1000 µF ]
                                             25V Low-ESR
                                                   |
                                                  GND
```

---

# Part 2: Handheld Remote Controller (ESP32 Standard)

The remote uses a standard **ESP32 DevKit V1 (30-pin or 38-pin)** with tactile switches, an I2C OLED, an EC11 rotary encoder, and a haptic vibration motor.

## 2.1 Complete GPIO Pin Assignment Table

| Remote GPIO | Device | Function | Wiring Type |
|---|---|---|---|
| **GPIO 13** | Upper Left Button | **Left Forward** | Pushbutton to GND (`INPUT_PULLUP`) |
| **GPIO 12** | Lower Left Button | **Left Reverse** | Pushbutton to GND (`INPUT_PULLUP`) |
| **GPIO 14** | Upper Right Button | **Right Forward** | Pushbutton to GND (`INPUT_PULLUP`) |
| **GPIO 27** | Lower Right Button | **Right Reverse** | Pushbutton to GND (`INPUT_PULLUP`) |
| **GPIO 32** | Trigger 1 Button | **Speed Preset 1 Override** | Pushbutton to GND (`INPUT_PULLUP`) |
| **GPIO 33** | Trigger 2 Button | **Speed Preset 2 Override** | Pushbutton to GND (`INPUT_PULLUP`) |
| **GPIO 15** | Coin Vibration Motor | **Haptic Feedback** | 2N2222 NPN transistor driver |
| **GPIO 25** | EC11 Rotary Encoder | `CLK` (Phase A) | Encoder pin with 10k pull-up |
| **GPIO 26** | EC11 Rotary Encoder | `DT` (Phase B) | Encoder pin with 10k pull-up |
| **GPIO 4**  | EC11 Rotary Encoder | `SW` (Push Switch) | Encoder click button (`INPUT_PULLUP`) |
| **GPIO 21** | 0.96" OLED Display | `SDA` (I2C Data) | Standard I2C Data line |
| **GPIO 22** | 0.96" OLED Display | `SCL` (I2C Clock) | Standard I2C Clock line |
| **3.3V**    | Display & Encoder | Logic Power Supply | Powers OLED and Encoder VCC |
| **GND**     | All Peripherals | Common Ground | Common return path |
