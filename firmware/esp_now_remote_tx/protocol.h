#ifndef ROBO_PROTOCOL_H
#define ROBO_PROTOCOL_H

#include <stdint.h>

#define PROTOCOL_MAGIC_BYTE 0x52  // 'R' for RoboRace
#define PROTOCOL_VERSION    0x02  // Protocol v2.0

// ============================================================================
// 1. CONTROL PACKET: Remote Controller (TX) -> Robot Receiver (RX)
// Frequency: 200 Hz (Every 5 ms) | Size: 9 bytes
// ============================================================================
typedef struct __attribute__((packed)) {
    uint8_t  magic;             // Protocol validation (0x52)
    int16_t  left_motor_speed;  // -255 (Full Rev) to +255 (Full Fwd)
    int16_t  right_motor_speed; // -255 (Full Rev) to +255 (Full Fwd)
    uint16_t target_rpm;        // Active global RPM cap (e.g. 400, 500, 600)
    uint8_t  brake_flag;        // 1 = Active electronic brake, 0 = Drive
    uint8_t  packet_id;         // Rolling sequence counter (0 - 255)
} BotControlPacket;

// ============================================================================
// 2. TELEMETRY PACKET: Robot Receiver (RX) -> Remote Controller (TX)
// Frequency: 10 Hz (Every 100 ms) | Size: 18 bytes
// ============================================================================
typedef struct __attribute__((packed)) {
    uint8_t  magic;             // Protocol validation (0x52)
    uint16_t cell1_mv;          // Cell 1 voltage in mV (e.g. 4180 = 4.18V)
    uint16_t cell2_mv;          // Cell 2 voltage in mV (e.g. 4180 = 4.18V)
    uint16_t cell3_mv;          // Cell 3 voltage in mV (e.g. 4180 = 4.18V)
    uint16_t total_bat_mv;      // Total pack voltage in mV (e.g. 12540 = 12.54V)
    uint16_t left_current_ma;   // Left BTS7960 current in mA
    uint16_t right_current_ma;  // Right BTS7960 current in mA
    int8_t   temp_battery_c;    // DS18B20 1: LiPo core temperature in °C
    int8_t   temp_left_drv_c;   // DS18B20 2: Left BTS7960 heatsink temp in °C
    int8_t   temp_right_drv_c;  // DS18B20 3: Right BTS7960 heatsink temp in °C
    uint8_t  battery_percent;   // Calculated remaining battery % (0 - 100%)
    uint8_t  status_flags;      // Bit 0: Low-voltage warning
                                // Bit 1: Over-temperature warning
                                // Bit 2: Inverted / Flipped state (1 = Flipped)
                                // Bit 3: Motor stall warning
} BotTelemetryPacket;

// Status Flag Bitmasks
#define FLAG_LOW_VOLTAGE  (1 << 0)
#define FLAG_OVER_TEMP    (1 << 1)
#define FLAG_IS_FLIPPED   (1 << 2)
#define FLAG_MOTOR_STALL  (1 << 3)

#endif // ROBO_PROTOCOL_H
