/**
 * ============================================================================
 * ROBO RACE BOT WIRELESS — ROBOT RECEIVER (RX) FIRMWARE
 * Target MCU: Espressif ESP32-S3-WROOM-1U-N16
 * Protocol: Bare-Metal ESP-NOW (2.4 GHz)
 * Architecture: FreeRTOS Asymmetric Dual-Core (Reflex vs. Housekeeping)
 * ============================================================================
 */

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <esp_arduino_version.h>

#if __has_include("protocol.h")
#include "protocol.h"
#elif __has_include("../common/protocol.h")
#include "../common/protocol.h"
#endif

// ============================================================================
// HARDWARE PIN DEFINITIONS (ESP32-S3)
// ============================================================================
// Left BTS7960 Motor Driver
#define PIN_LEFT_RPWM       17    // Forward PWM
#define PIN_LEFT_LPWM       18    // Reverse PWM
#define PIN_LEFT_EN         15    // Driver Enable (R_EN + L_EN bridged)

// Right BTS7960 Motor Driver
#define PIN_RIGHT_RPWM      38    // Forward PWM
#define PIN_RIGHT_LPWM      39    // Reverse PWM
#define PIN_RIGHT_EN        40    // Driver Enable (R_EN + L_EN bridged)

// MPU-6050 I2C Orientation Sensor
#define PIN_I2C_SDA         1     // I2C Data
#define PIN_I2C_SCL         2     // I2C Clock
#define MPU6050_ADDR        0x68  // Default I2C Address

// 3S Balance Port ADC Channels (ADC1)
#define PIN_ADC_CELL1       4     // ADC1_CH3 (0 - 4.2V via 10k/10k)
#define PIN_ADC_CELL2       5     // ADC1_CH4 (0 - 8.4V via 30k/10k)
#define PIN_ADC_CELL3       6     // ADC1_CH5 (0 - 12.6V via 50k/10k)

// BTS7960 Current Sense ADC Channels (ADC1)
#define PIN_ADC_LEFT_IS     7     // ADC1_CH6 (Left motor current)
#define PIN_ADC_RIGHT_IS    8     // ADC1_CH7 (Right motor current)

// 1-Wire Digital Temperatures (DS18B20)
#define PIN_ONEWIRE_BUS     14    // 3x DS18B20 digital bus

// PWM Hardware Configuration (20 kHz Ultrasonic Frequency)
#define PWM_FREQ            20000 // 20 kHz (Inaudible, instant coil saturation)
#define PWM_RES             8     // 8-bit resolution (0 - 255)

// Voltage Divider Calibration Multipliers
#define RATIO_CELL1         2.000f // 10k + 10k / 10k
#define RATIO_CELL2         4.000f // 30k + 10k / 10k
#define RATIO_CELL3         6.000f // 50k + 10k / 10k

// ============================================================================
// CROSS-VERSION COMPATIBILITY MACROS (Arduino ESP32 Core 2.x & Core 3.x)
// ============================================================================
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  // Arduino ESP32 Core 3.x API (pin-based LEDC and DB_12 ADC)
  #define MOTOR_PWM_INIT() do { \
      ledcAttach(PIN_LEFT_RPWM, PWM_FREQ, PWM_RES); \
      ledcAttach(PIN_LEFT_LPWM, PWM_FREQ, PWM_RES); \
      ledcAttach(PIN_RIGHT_RPWM, PWM_FREQ, PWM_RES); \
      ledcAttach(PIN_RIGHT_LPWM, PWM_FREQ, PWM_RES); \
  } while(0)
  #define WRITE_LEFT_RPWM(val)   ledcWrite(PIN_LEFT_RPWM, val)
  #define WRITE_LEFT_LPWM(val)   ledcWrite(PIN_LEFT_LPWM, val)
  #define WRITE_RIGHT_RPWM(val)  ledcWrite(PIN_RIGHT_RPWM, val)
  #define WRITE_RIGHT_LPWM(val)  ledcWrite(PIN_RIGHT_LPWM, val)
  #define ADC_MAX_ATTEN          ADC_ATTEN_DB_12
#else
  // Arduino ESP32 Core 2.x API (channel-based LEDC and 11db ADC)
  #define CH_LEFT_RPWM   0
  #define CH_LEFT_LPWM   1
  #define CH_RIGHT_RPWM  2
  #define CH_RIGHT_LPWM  3
  #define MOTOR_PWM_INIT() do { \
      ledcSetup(CH_LEFT_RPWM, PWM_FREQ, PWM_RES); \
      ledcAttachPin(PIN_LEFT_RPWM, CH_LEFT_RPWM); \
      ledcSetup(CH_LEFT_LPWM, PWM_FREQ, PWM_RES); \
      ledcAttachPin(PIN_LEFT_LPWM, CH_LEFT_LPWM); \
      ledcSetup(CH_RIGHT_RPWM, PWM_FREQ, PWM_RES); \
      ledcAttachPin(PIN_RIGHT_RPWM, CH_RIGHT_RPWM); \
      ledcSetup(CH_RIGHT_LPWM, PWM_FREQ, PWM_RES); \
      ledcAttachPin(PIN_RIGHT_LPWM, CH_RIGHT_LPWM); \
  } while(0)
  #define WRITE_LEFT_RPWM(val)   ledcWrite(CH_LEFT_RPWM, val)
  #define WRITE_LEFT_LPWM(val)   ledcWrite(CH_LEFT_LPWM, val)
  #define WRITE_RIGHT_RPWM(val)  ledcWrite(CH_RIGHT_RPWM, val)
  #define WRITE_RIGHT_LPWM(val)  ledcWrite(CH_RIGHT_LPWM, val)
  #define ADC_MAX_ATTEN          ADC_11db
#endif

// ============================================================================
// GLOBAL VOLATILE SHARED STATE (Atomic lock-free exchange between cores)
// ============================================================================
volatile BotControlPacket   g_controlPacket;
volatile BotTelemetryPacket g_telemetryPacket;
volatile uint32_t           g_lastPacketTime = 0;
volatile bool               g_isFlipped = false;
volatile bool               g_failsafeActive = true;

// Remote MAC Address storage (Learned on first packet or paired)
uint8_t g_remoteMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool    g_remotePaired = false;

// 1-Wire Sensors Setup (Core 0 only)
OneWire oneWire(PIN_ONEWIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// Tasks
TaskHandle_t hReflexTask;
TaskHandle_t hHousekeepingTask;

// Forward Declarations
void applyMotorsBreakBeforeMake(int16_t left, int16_t right);
void initMPU6050();
void readMPUOrientation();
void readBatteryAndCurrent();
void readTemperaturesAsync();

// ============================================================================
// ESP-NOW RECEIVE CALLBACK (Cross-version compatible Core 2.x and Core 3.x)
// ============================================================================
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *data, int len) {
    const uint8_t *srcMac = recv_info->src_addr;
#else
void OnDataRecv(const uint8_t *srcMac, const uint8_t *data, int len) {
#endif
    if (len == sizeof(BotControlPacket)) {
        BotControlPacket* pkt = (BotControlPacket*)data;
        if (pkt->magic == PROTOCOL_MAGIC_BYTE) {
            memcpy((void*)&g_controlPacket, data, sizeof(BotControlPacket));
            g_lastPacketTime = millis();
            g_failsafeActive = false;

            // Learn remote MAC address for bidirectional telemetry
            if (!g_remotePaired) {
                memcpy(g_remoteMac, srcMac, 6);
                esp_now_peer_info_t peerInfo = {};
                memcpy(peerInfo.peer_addr, g_remoteMac, 6);
                peerInfo.channel = 1;
                peerInfo.encrypt = false;
                if (esp_now_add_peer(&peerInfo) == ESP_OK) {
                    g_remotePaired = true;
                }
            }
        }
    }
}

// ============================================================================
// CORE 1: THE DEDICATED REFLEX MOTOR TASK (200 Hz - Priority 24)
// ============================================================================
void motorReflexTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(5); // 200 Hz (Every 5 ms)

    for (;;) {
        uint32_t now = millis();

        // 1. Check Failsafe Watchdog (100 ms timeout)
        if ((now - g_lastPacketTime) > 100 || g_failsafeActive) {
            applyMotorsBreakBeforeMake(0, 0); // Active electronic brake
            g_failsafeActive = true;
        } else {
            // 2. Decode Control Inputs
            int16_t desiredLeft  = g_controlPacket.left_motor_speed;
            int16_t desiredRight = g_controlPacket.right_motor_speed;

            // 3. ZERO-LAG AUTO-INVERT FLIP REMAPPING (1 CPU cycle execution)
            int16_t actualLeft, actualRight;
            if (g_isFlipped) {
                // Swaps Left <-> Right AND inverts direction polarity
                actualLeft  = -desiredRight;
                actualRight = -desiredLeft;
            } else {
                actualLeft  = desiredLeft;
                actualRight = desiredRight;
            }

            // 4. Apply Break-Before-Make 20 kHz PWM
            if (g_controlPacket.brake_flag) {
                applyMotorsBreakBeforeMake(0, 0);
            } else {
                applyMotorsBreakBeforeMake(actualLeft, actualRight);
            }
        }

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

// ============================================================================
// HARDWARE BREAK-BEFORE-MAKE MOTOR DRIVE (Shoot-Through Protection)
// Cross-version compatible with Arduino ESP32 Core 2.x and 3.x
// ============================================================================
void applyMotorsBreakBeforeMake(int16_t left, int16_t right) {
    left  = constrain(left, -255, 255);
    right = constrain(right, -255, 255);

    // Left Motor Channel
    if (left > 0) {
        WRITE_LEFT_LPWM(0);
        esp_rom_delay_us(3); // 3 µs hardware dead-time to prevent shoot-through
        WRITE_LEFT_RPWM(left);
    } else if (left < 0) {
        WRITE_LEFT_RPWM(0);
        esp_rom_delay_us(3);
        WRITE_LEFT_LPWM(-left);
    } else {
        // Electronic Active Brake: both LOW
        WRITE_LEFT_RPWM(0);
        WRITE_LEFT_LPWM(0);
    }

    // Right Motor Channel
    if (right > 0) {
        WRITE_RIGHT_LPWM(0);
        esp_rom_delay_us(3);
        WRITE_RIGHT_RPWM(right);
    } else if (right < 0) {
        WRITE_RIGHT_RPWM(0);
        esp_rom_delay_us(3);
        WRITE_RIGHT_LPWM(-right);
    } else {
        // Electronic Active Brake: both LOW
        WRITE_RIGHT_RPWM(0);
        WRITE_RIGHT_LPWM(0);
    }
}

// ============================================================================
// CORE 0: THE BACKGROUND HOUSEKEEPING TASK (Priority 1)
// ============================================================================
void housekeepingTask(void *pvParameters) {
    uint32_t lastTelemetryTime = 0;
    uint32_t lastTempTime = 0;
    uint32_t lastMpuTime = 0;

    initMPU6050();

    for (;;) {
        uint32_t now = millis();

        // 1. MPU-6050 Orientation Read (50 Hz / 20 ms)
        if (now - lastMpuTime >= 20) {
            readMPUOrientation();
            lastMpuTime = now;
        }

        // 2. Battery & Current Sensing (20 Hz / 50 ms)
        if (now - lastTelemetryTime >= 50) {
            readBatteryAndCurrent();
        }

        // 3. DS18B20 Temperatures (1 Hz / 1000 ms - Non-blocking)
        if (now - lastTempTime >= 1000) {
            readTemperaturesAsync();
            lastTempTime = now;
        }

        // 4. Send Telemetry Packet to Remote (10 Hz / 100 ms)
        if (now - lastTelemetryTime >= 100) {
            if (g_remotePaired) {
                g_telemetryPacket.magic = PROTOCOL_MAGIC_BYTE;
                if (g_isFlipped) g_telemetryPacket.status_flags |= FLAG_IS_FLIPPED;
                else             g_telemetryPacket.status_flags &= ~FLAG_IS_FLIPPED;

                esp_now_send(g_remoteMac, (uint8_t*)&g_telemetryPacket, sizeof(BotTelemetryPacket));
            }
            lastTelemetryTime = now;
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Yield to prevent watchdog trip on Core 0
    }
}

// ============================================================================
// SENSOR ROUTINES (Running safely on Core 0)
// ============================================================================
void initMPU6050() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000); // 400 kHz Fast I2C
    Wire.setTimeOut(2); // 2 ms HARDWARE TIMEOUT: Prevents CPU lockup!

    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x6B); // PWR_MGMT_1
    Wire.write(0x00); // Wake up MPU
    Wire.endTransmission();
}

void readMPUOrientation() {
    static uint32_t flipDebounceTimer = 0;
    static bool flipCandidate = false;

    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x3F); // ACCEL_ZOUT_H
    if (Wire.endTransmission(false) == 0) {
        if (Wire.requestFrom((uint8_t)MPU6050_ADDR, (size_t)2) == 2) {
            int16_t rawZ = (Wire.read() << 8) | Wire.read();
            float az = rawZ / 16384.0f; // Scale to Gs (at +/-2g range)

            // Flipped state: Z-axis gravity becomes negative (< -0.35g)
            if (az < -0.35f) {
                if (!flipCandidate) {
                    flipCandidate = true;
                    flipDebounceTimer = millis();
                } else if (millis() - flipDebounceTimer > 50) { // 50ms anti-bounce filter
                    g_isFlipped = true;
                }
            } else if (az > +0.35f) {
                if (flipCandidate) {
                    flipCandidate = false;
                    flipDebounceTimer = millis();
                } else if (millis() - flipDebounceTimer > 50) {
                    g_isFlipped = false;
                }
            }
        }
    }
}

void readBatteryAndCurrent() {
    static float filteredTotalMv = 12600.0f;

    // Use ESP32-S3 factory eFuse calibrated analogReadMilliVolts!
    // Eliminates raw ADC non-linearities automatically
    uint32_t mv1 = analogReadMilliVolts(PIN_ADC_CELL1);
    uint32_t mv2 = analogReadMilliVolts(PIN_ADC_CELL2);
    uint32_t mv3 = analogReadMilliVolts(PIN_ADC_CELL3);

    float v1 = mv1 * RATIO_CELL1;
    float v2 = mv2 * RATIO_CELL2;
    float v3 = mv3 * RATIO_CELL3;

    uint16_t c1 = (uint16_t)v1;
    uint16_t c2 = (uint16_t)max(0.0f, v2 - v1);
    uint16_t c3 = (uint16_t)max(0.0f, v3 - v2);
    uint16_t total = (uint16_t)v3;

    // 10-sample Exponential Moving Average (EMA) to ignore acceleration dips
    filteredTotalMv = (0.90f * filteredTotalMv) + (0.10f * total);

    g_telemetryPacket.cell1_mv     = c1;
    g_telemetryPacket.cell2_mv     = c2;
    g_telemetryPacket.cell3_mv     = c3;
    g_telemetryPacket.total_bat_mv = (uint16_t)filteredTotalMv;

    // Battery Percentage calculation (3S LiPo curve: 10.2V = 0%, 12.6V = 100%)
    float pct = ((filteredTotalMv - 10200.0f) / (12600.0f - 10200.0f)) * 100.0f;
    g_telemetryPacket.battery_percent = (uint8_t)constrain(pct, 0.0f, 100.0f);

    // Motor Current Sensing using calibrated millivolts
    uint32_t mvCurL = analogReadMilliVolts(PIN_ADC_LEFT_IS);
    uint32_t mvCurR = analogReadMilliVolts(PIN_ADC_RIGHT_IS);

    // 1k shunt: V = I * 1000 / 8500 => I(mA) = V(mV) * 8.5
    g_telemetryPacket.left_current_ma  = (uint16_t)(mvCurL * 8.5f);
    g_telemetryPacket.right_current_ma = (uint16_t)(mvCurR * 8.5f);

    // Low voltage flag
    if (filteredTotalMv < 10200.0f || c1 < 3300 || c2 < 3300 || c3 < 3300) {
        g_telemetryPacket.status_flags |= FLAG_LOW_VOLTAGE;
    } else {
        g_telemetryPacket.status_flags &= ~FLAG_LOW_VOLTAGE;
    }
}

void readTemperaturesAsync() {
    // Non-blocking DS18B20: collect previous read, request new conversion
    float tBat  = ds18b20.getTempCByIndex(0);
    float tDrvL = ds18b20.getTempCByIndex(1);
    float tDrvR = ds18b20.getTempCByIndex(2);

    if (tBat > -55.0f && tBat < 125.0f)  g_telemetryPacket.temp_battery_c   = (int8_t)tBat;
    if (tDrvL > -55.0f && tDrvL < 125.0f) g_telemetryPacket.temp_left_drv_c  = (int8_t)tDrvL;
    if (tDrvR > -55.0f && tDrvR < 125.0f) g_telemetryPacket.temp_right_drv_c = (int8_t)tDrvR;

    // Over-temperature alert: Battery > 50°C or Drivers > 80°C
    if (tBat > 50.0f || tDrvL > 80.0f || tDrvR > 80.0f) {
        g_telemetryPacket.status_flags |= FLAG_OVER_TEMP;
    } else {
        g_telemetryPacket.status_flags &= ~FLAG_OVER_TEMP;
    }

    // Request new conversion asynchronously (non-blocking)
    ds18b20.requestTemperatures();
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);

    // 1. Configure Motor GPIOs & Enables
    pinMode(PIN_LEFT_EN, OUTPUT);
    pinMode(PIN_RIGHT_EN, OUTPUT);
    digitalWrite(PIN_LEFT_EN, HIGH);  // Enable BTS7960 Left
    digitalWrite(PIN_RIGHT_EN, HIGH); // Enable BTS7960 Right

    // 2. Hardware PWM Setup (20 kHz, 8-bit) - Universal Core 2.x & 3.x
    MOTOR_PWM_INIT();
    applyMotorsBreakBeforeMake(0, 0); // Start locked in brake mode

    // 3. Configure ADC Attenuation (Universal Core 2.x & 3.x)
    analogSetPinAttenuation(PIN_ADC_CELL1, ADC_MAX_ATTEN);
    analogSetPinAttenuation(PIN_ADC_CELL2, ADC_MAX_ATTEN);
    analogSetPinAttenuation(PIN_ADC_CELL3, ADC_MAX_ATTEN);
    analogSetPinAttenuation(PIN_ADC_LEFT_IS, ADC_MAX_ATTEN);
    analogSetPinAttenuation(PIN_ADC_RIGHT_IS, ADC_MAX_ATTEN);

    // 4. Configure DS18B20 1-Wire (Non-blocking)
    ds18b20.begin();
    ds18b20.setWaitForConversion(false); // CRITICAL: Non-blocking mode!
    ds18b20.requestTemperatures();

    // 5. Bare-Metal High-Speed ESP-NOW Setup
    WiFi.mode(WIFI_STA);
    esp_wifi_set_ps(WIFI_PS_NONE); // Modem power saving DISABLED (Full Power)
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Init Failed!");
        return;
    }
    esp_now_register_recv_cb(OnDataRecv);

    // 6. Launch FreeRTOS Tasks across Dual Cores
    // Core 1: Dedicated Reflex Motor Task (Priority 24 - Maximum Real-Time)
    xTaskCreatePinnedToCore(
        motorReflexTask, "MotorReflex", 4096, NULL, 24, &hReflexTask, 1
    );

    // Core 0: Dedicated Housekeeping & Sensor Task (Priority 1 - Background)
    xTaskCreatePinnedToCore(
        housekeepingTask, "Housekeeping", 4096, NULL, 1, &hHousekeepingTask, 0
    );

    Serial.println("ROBO Race Bot RX Armed & Ready (Core 1: Motors | Core 0: Sensors)");
}

void loop() {
    // Background idle delay to keep Core 0 loop alive without starving other tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
}
