# ROBO Race Bot Wireless — Master Architecture Repository

A high-performance, competition-ready **4WD Wireless Race Bot** and **Handheld Remote Controller** built on the **Espressif ESP32-S3** and **ESP-NOW** protocol for sub-2ms reflex latency, high torque delivery, multi-cell battery telemetry, and full thermal safety.

---

## 🏎️ Core Hardware Specifications

| Component | Hardware Specification | Key Role / Capability |
|---|---|---|
| **Robot MCU** | **Espressif ESP32-S3-WROOM-1U-N16** | 240MHz Dual-Core LX7, 16MB Flash, **External U.FL Antenna**, Native USB-C |
| **Remote MCU**| **Standard ESP32 DevKit V1** | 240MHz Dual-Core, I2C OLED display, EC11 rotary encoder, 6x tactile buttons |
| **Motors** | **4x Johnson 12V 600RPM Geared Motors** | 4.53 kg·cm stall torque each (**18.12 kg·cm combined**), 2 parallel per side |
| **Motor Drivers**| **2x BTS7960 43A High-Power H-Bridges** | Independent Left & Right drivers, $16\text{ m}\Omega$ MOSFETs, zero voltage drop |
| **Battery** | **3S 11.1V–12.6V 2200mAh LiPo (40C–80C)** | Delivers 88A–176A burst, sealed inside chassis with external charging ports |
| **Sensors** | **3x DS18B20 1-Wire Digital Temps** | Real-time monitoring of LiPo battery and both BTS7960 heatsinks |
| **Sensors** | **3-Cell Precision Balance Port Dividers**| Monitors individual cells (Cell 1, 2, 3), total pack voltage, and health |
| **Sensors** | **Dual BTS7960 Current Sense (`IS`)** | Real-time motor load telemetry and stall/burnout protection |
| **Protocol** | **Bare-Metal ESP-NOW (2.4 GHz)** | **200 Hz packet rate (< 2ms latency)**, zero Wi-Fi/router overhead |

---

## 📁 Repository Structure

```
robo_race_bot_wireless/
├── architecture/
│   ├── SYSTEM_ARCHITECTURE.md          # Multi-core FreeRTOS model, power flow & failsafes
│   ├── HARDWARE_PINOUT_AND_WIRING.md   # Complete GPIO pinout tables, circuits & diagrams
│   ├── REMOTE_CONTROLLER_SPEC.md       # 4-button tank drive, dual triggers & OLED menus
│   └── COMMUNICATION_PROTOCOL.md       # ESP-NOW C-structures, 200Hz timing & watchdog
├── hardware/
│   ├── BOM.md                          # Full Bill of Materials with supplier links & specs
│   └── WIRING_AND_PINOUT.md            # Circuit schematics & power distribution guide
├── firmware/
│   ├── esp32_wifi_bot/                 # Standalone Wi-Fi fallback firmware
│   ├── esp_now_bot_rx/                 # Main ESP-NOW 200Hz Robot Receiver firmware
│   └── esp_now_remote_tx/              # Handheld Remote Controller firmware
├── controller_web/
│   └── index.html                      # Standalone browser/gamepad cockpit controller
├── .gitignore
└── README.md                           # Master Project Overview
```

---

## 🎮 Driving & Control Architecture

### 4-Button Tank Drive
* **Upper Left + Upper Right:** Full Straight Forward
* **Lower Left + Lower Right:** Full Straight Reverse
* **Upper Left + Lower Right:** 360° Clockwise Pivot Spin
* **Upper Right + Lower Left:** 360° Counter-Clockwise Pivot Spin
* **Single Button:** Sweeping pivot turn

### Dual-Trigger Global Speed Overrides
* **No Triggers Held:** Movement runs at **Base Speed** (default: 400 RPM).
* **Hold Trigger 1:** Movement runs at **Preset 1 Speed** (default: 500 RPM).
* **Hold Trigger 2:** Movement runs at **Preset 2 Speed** (default: 600 RPM Full Punch).
* *Settings are tuned via the remote's rotary encoder and saved directly to the remote's flash memory!*

---

## 🔋 Battery & Safety Management

1. **All-in-One Sealed Chassis:** The 3S LiPo is permanently secured inside.
2. **External Ports:**
   * **XT60 Male Socket:** High-current charging port.
   * **3S JST-XH 4-Pin:** Cell balance monitoring during charging.
   * **Master Power Switch:** 30A mechanical power isolation.
   * **USB Type-C Socket:** Connected directly to ESP32-S3 GPIO 19 (`D-`) and GPIO 20 (`D+`) for one-click firmware updates without opening the chassis.
3. **Telemetry to Remote:** Real-time cell voltages, battery percentage, remaining race minutes, motor current (Amps), and 3-point temperatures (°C) stream live to the remote's OLED screen.

---

## 📄 License & Credits
Designed and engineered for competitive robotics racing. Open-source under the MIT License.
