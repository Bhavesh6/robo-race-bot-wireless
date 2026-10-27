# ESP-NOW Communication Protocol Specification

## 1. Protocol Architecture

The **ROBO Race Bot Wireless** uses the **ESP-NOW** protocol from Espressif Systems:
* **Operating Layer:** Direct 2.4 GHz MAC-layer frame exchange (IEEE 802.11 Action Frames).
* **Latency:** Sub-1.5 to 2.5 milliseconds end-to-end.
* **No Router / No Handshake:** Peer-to-peer connection without IP routing or DHCP delays.
* **Range:** ~150 to 200+ meters outdoors with the ESP32-S3 external U.FL antenna.

```
       REMOTE CONTROLLER (TX)                            BOT RECEIVER (RX)
                 |                                               |
                 |-------- [ CONTROL PACKET (200 Hz) ] --------->|
                 |         (Every 5 ms: Speed, Steering, Brake)  |
                 |                                               | (Executes Motors)
                 |                                               |
                 |<------- [ TELEMETRY PACKET (10 Hz) ] ---------|
                 |         (Every 100 ms: Voltage, Amps, Temps)  |
                 |                                               |
```

---

## 2. Packet Data Structures (C-Structures)

Both devices compile the identical data structures to guarantee byte alignment.

### 2.1 Control Packet: Remote $\rightarrow$ Bot (200 Hz)

```c
// Size: 8 bytes (Ultra-compact for sub-1ms transmission)
typedef struct __attribute__((packed)) {
    int16_t left_motor_speed;   // -255 to +255 (Reverse to Forward PWM)
    int16_t right_motor_speed;  // -255 to +255 (Reverse to Forward PWM)
    uint16_t target_rpm;        // Current active RPM cap (e.g. 400, 500, 600)
    uint8_t  brake_flag;        // 1 = Active electronic brake, 0 = Drive
    uint8_t  packet_id;         // Rolling sequence counter (0 - 255)
} BotControlPacket;
```

### 2.2 Telemetry Packet: Bot $\rightarrow$ Remote (10 Hz)

```c
// Size: 18 bytes
typedef struct __attribute__((packed)) {
    uint16_t cell1_mv;          // Cell 1 voltage in millivolts (e.g. 4180 = 4.18V)
    uint16_t cell2_mv;          // Cell 2 voltage in millivolts (e.g. 4180 = 4.18V)
    uint16_t cell3_mv;          // Cell 3 voltage in millivolts (e.g. 4180 = 4.18V)
    uint16_t total_bat_mv;      // Total pack voltage in millivolts (e.g. 12540 = 12.54V)
    uint16_t left_current_ma;   // Left motor driver current draw in mA
    uint16_t right_current_ma;  // Right motor driver current draw in mA
    int8_t   temp_battery_c;    // DS18B20 1: LiPo core temperature in °C
    int8_t   temp_left_drv_c;   // DS18B20 2: Left BTS7960 heatsink temp in °C
    int8_t   temp_right_drv_c;  // DS18B20 3: Right BTS7960 heatsink temp in °C
    uint8_t  battery_percent;   // Calculated remaining battery percentage (0 - 100%)
    uint8_t  status_flags;      // Bit 0: Low-voltage warning, Bit 1: Over-temp warning
} BotTelemetryPacket;
```

---

## 3. Radio Optimization Settings

To achieve **zero latency**, the firmware initializes the Wi-Fi modem with bare-metal parameters:

```cpp
// 1. Force station mode
WiFi.mode(WIFI_STA);

// 2. CRITICAL: Disable modem power saving (modem stays 100% active)
esp_wifi_set_ps(WIFI_PS_NONE);

// 3. Lock to high-speed 2.4 GHz channel
esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

// 4. Set maximum transmission power (+20 dBm)
WiFi.setTxPower(WIFI_POWER_19_5dBm);
```

---

## 4. Failsafe Watchdog Engine

If the remote is powered off, goes out of range, or suffers interference:
* A hardware timer on **Core 1** checks the timestamp of the last valid `BotControlPacket`.
* If `(millis() - last_packet_time) > 100 ms`:
  1. Both BTS7960 drivers are immediately set to **0% PWM**.
  2. Electronic active braking engages to bring the robot to an immediate halt.
  3. Status LED / buzzer emits a signal-loss warning pulse.
* The moment a new packet arrives, the watchdog resets and normal driving resumes instantly.
