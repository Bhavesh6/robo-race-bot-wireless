/**
 * ============================================================================
 * ROBO RACE BOT WIRELESS — HANDHELD REMOTE CONTROLLER (TX) FIRMWARE
 * Target MCU: Standard ESP32 DevKit V1
 * Protocol: Bare-Metal ESP-NOW (2.4 GHz)
 * Features: 4-Button Tank Drive + Dual Speed Triggers + 0.96" OLED + EC11 Encoder
 * ============================================================================
 */

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#if __has_include("protocol.h")
#include "protocol.h"
#elif __has_include("../common/protocol.h")
#include "../common/protocol.h"
#endif

// ============================================================================
// HARDWARE PIN DEFINITIONS (ESP32 Standard)
// ============================================================================
// 4-Button Tank Drive Controls (Active LOW with INPUT_PULLUP)
#define BTN_UPPER_LEFT      13    // Left Forward
#define BTN_LOWER_LEFT      12    // Left Reverse
#define BTN_UPPER_RIGHT     14    // Right Forward
#define BTN_LOWER_RIGHT     27    // Right Reverse

// Dual Speed Trigger Overrides (Active LOW with INPUT_PULLUP)
#define BTN_TRIGGER_1       32    // Speed Preset 1 Override
#define BTN_TRIGGER_2       33    // Speed Preset 2 Override (Turbo)

// Haptic Vibration Motor (Driven via NPN Transistor)
#define PIN_HAPTIC          15

// EC11 Rotary Encoder
#define PIN_ENCODER_CLK     25
#define PIN_ENCODER_DT      26
#define PIN_ENCODER_SW      4     // Encoder Push Switch

// I2C OLED Display (128x64 SSD1306)
#define PIN_OLED_SDA        21
#define PIN_OLED_SCL        22
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64
#define OLED_RESET          -1

// ============================================================================
// CONFIGURATION & PERSISTENT NVS STORAGE
// ============================================================================
Preferences prefs;

struct UserSettings {
    uint16_t speed_base;    // Default RPM (e.g. 400)
    uint16_t speed_trig1;   // Trigger 1 RPM (e.g. 500)
    uint16_t speed_trig2;   // Trigger 2 RPM (e.g. 600)
} g_settings;

// Bot Broadcast / Paired MAC Address (Broadcasts on first run or fixed peer)
uint8_t g_botMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Packets
BotControlPacket   g_ctrlPkt;
BotTelemetryPacket g_telemPkt;
volatile uint32_t  g_lastTelemTime = 0;
uint8_t            g_packetSeq = 0;

// Display & UI State
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
uint8_t  g_uiPage = 0;       // 0: Live Dashboard, 1: Speed Config
uint8_t  g_editCursor = 0;   // 0: Base, 1: Trig1, 2: Trig2
bool     g_inEditMode = false;
uint32_t g_hapticEndTime = 0;
bool     g_lastTrig1 = false;
bool     g_lastTrig2 = false;

// Encoder State
int  g_lastEncoderClk = HIGH;

// ============================================================================
// HAPTIC FEEDBACK ROUTINES
// ============================================================================
void triggerHaptic(uint32_t durationMs) {
    digitalWrite(PIN_HAPTIC, HIGH);
    g_hapticEndTime = millis() + durationMs;
}

void updateHaptic() {
    if (g_hapticEndTime > 0 && millis() >= g_hapticEndTime) {
        digitalWrite(PIN_HAPTIC, LOW);
        g_hapticEndTime = 0;
    }
}

// ============================================================================
// ESP-NOW TELEMETRY RECEIVE CALLBACK (Cross-version compatible)
// ============================================================================
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *data, int len) {
    const uint8_t *srcMac = recv_info->src_addr;
#else
void OnDataRecv(const uint8_t *srcMac, const uint8_t *data, int len) {
#endif
    if (len == sizeof(BotTelemetryPacket)) {
        BotTelemetryPacket* pkt = (BotTelemetryPacket*)data;
        if (pkt->magic == PROTOCOL_MAGIC_BYTE) {
            memcpy(&g_telemPkt, data, sizeof(BotTelemetryPacket));
            g_lastTelemTime = millis();

            // Auto-pair with bot's specific MAC
            if (!esp_now_is_peer_exist(srcMac)) {
                esp_now_peer_info_t peer = {};
                memcpy(peer.peer_addr, srcMac, 6);
                peer.channel = 1;
                peer.encrypt = false;
                if (esp_now_add_peer(&peer) == ESP_OK) {
                    memcpy(g_botMac, srcMac, 6);
                }
            } else {
                memcpy(g_botMac, srcMac, 6);
            }

            // Haptic alert on critical warnings
            if (pkt->status_flags & FLAG_LOW_VOLTAGE || pkt->status_flags & FLAG_OVER_TEMP) {
                triggerHaptic(150);
            }
        }
    }
}

// ============================================================================
// SPEED TIER RESOLUTION (Global Speed Override)
// ============================================================================
uint16_t getActiveTargetRpm(bool t1, bool t2) {
    if (t2) return g_settings.speed_trig2; // Trigger 2 has top priority (600 RPM)
    if (t1) return g_settings.speed_trig1; // Trigger 1 has secondary priority (500 RPM)
    return g_settings.speed_base;          // Base Speed (400 RPM)
}

int16_t rpmToPwm(uint16_t rpm) {
    return (int16_t)constrain((rpm * 255) / 600, 50, 255);
}

// ============================================================================
// 4-BUTTON TANK DRIVE CONTROLLER
// ============================================================================
void sampleButtonsAndSend() {
    // Read 4 Directional Buttons (Active LOW)
    bool ul = (digitalRead(BTN_UPPER_LEFT) == LOW);
    bool ll = (digitalRead(BTN_LOWER_LEFT) == LOW);
    bool ur = (digitalRead(BTN_UPPER_RIGHT) == LOW);
    bool lr = (digitalRead(BTN_LOWER_RIGHT) == LOW);

    // Read 2 Global Speed Triggers (Active LOW)
    bool t1 = (digitalRead(BTN_TRIGGER_1) == LOW);
    bool t2 = (digitalRead(BTN_TRIGGER_2) == LOW);

    // Haptic pulse when a trigger is first engaged
    if (t1 && !g_lastTrig1) triggerHaptic(40);
    if (t2 && !g_lastTrig2) triggerHaptic(60);
    g_lastTrig1 = t1;
    g_lastTrig2 = t2;

    // Resolve Active Global Speed
    uint16_t targetRpm = getActiveTargetRpm(t1, t2);
    int16_t  pwmPower  = rpmToPwm(targetRpm);

    int16_t leftCmd = 0;
    int16_t rightCmd = 0;
    uint8_t brake = 0;

    // Resolve Tank Drive Kinematics
    if (ul && !ll) leftCmd = pwmPower;        // Left Fwd
    else if (ll && !ul) leftCmd = -pwmPower;   // Left Rev

    if (ur && !lr) rightCmd = pwmPower;       // Right Fwd
    else if (lr && !ur) rightCmd = -pwmPower;  // Right Rev

    // Active Electronic Brake if no movement buttons pressed
    if (!ul && !ll && !ur && !lr) {
        brake = 1;
    }

    // Build Control Packet
    g_ctrlPkt.magic             = PROTOCOL_MAGIC_BYTE;
    g_ctrlPkt.left_motor_speed  = leftCmd;
    g_ctrlPkt.right_motor_speed = rightCmd;
    g_ctrlPkt.target_rpm        = targetRpm;
    g_ctrlPkt.brake_flag        = brake;
    g_ctrlPkt.packet_id         = g_packetSeq++;

    // Broadcast or Unicast directly to bot
    esp_now_send(g_botMac, (uint8_t*)&g_ctrlPkt, sizeof(BotControlPacket));
}

// ============================================================================
// ROTARY ENCODER MENU NAVIGATION
// ============================================================================
void handleEncoder() {
    int currentClk = digitalRead(PIN_ENCODER_CLK);
    if (currentClk != g_lastEncoderClk && currentClk == LOW) {
        int dtVal = digitalRead(PIN_ENCODER_DT);
        int step = (dtVal != currentClk) ? 25 : -25; // 25 RPM step

        if (g_uiPage == 0) {
            // In Dashboard: Quick change base speed
            g_settings.speed_base = constrain(g_settings.speed_base + step, 200, 600);
        } else if (g_uiPage == 1) {
            // In Config Menu: Adjust selected preset
            if (g_editCursor == 0) g_settings.speed_base  = constrain(g_settings.speed_base + step, 200, 600);
            if (g_editCursor == 1) g_settings.speed_trig1 = constrain(g_settings.speed_trig1 + step, 200, 600);
            if (g_editCursor == 2) g_settings.speed_trig2 = constrain(g_settings.speed_trig2 + step, 200, 600);
        }
        triggerHaptic(15);
    }
    g_lastEncoderClk = currentClk;

    // Check Encoder Click Button
    static bool lastSwState = HIGH;
    bool swState = digitalRead(PIN_ENCODER_SW);
    if (swState == LOW && lastSwState == HIGH) {
        triggerHaptic(40);
        if (g_uiPage == 0) {
            g_uiPage = 1; // Enter Settings Menu
            g_editCursor = 0;
        } else if (g_uiPage == 1) {
            g_editCursor++;
            if (g_editCursor > 2) {
                // Save to NVS Flash and return to Dashboard
                prefs.putUShort("spd_base", g_settings.speed_base);
                prefs.putUShort("spd_t1", g_settings.speed_trig1);
                prefs.putUShort("spd_t2", g_settings.speed_trig2);
                g_uiPage = 0;
                triggerHaptic(100);
            }
        }
    }
    lastSwState = swState;
}

// ============================================================================
// OLED DISPLAY RENDERING (10 Hz)
// ============================================================================
void updateOledDisplay() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    if (g_uiPage == 0) {
        // --- PAGE 0: LIVE RACING DASHBOARD ---
        display.setTextSize(1);
        display.setCursor(0, 0);

        // Header: Battery & Status
        float batV = g_telemPkt.total_bat_mv / 1000.0f;
        display.printf("BAT:%.2fV  %3d%%", batV, g_telemPkt.battery_percent);

        // Flip indicator
        if (g_telemPkt.status_flags & FLAG_IS_FLIPPED) {
            display.setCursor(104, 0);
            display.print("[FLIP]");
        }

        display.drawLine(0, 9, 128, 9, SSD1306_WHITE);

        // Cells
        display.setCursor(0, 13);
        display.printf("C1:%.2f C2:%.2f C3:%.2f",
            g_telemPkt.cell1_mv / 1000.0f,
            g_telemPkt.cell2_mv / 1000.0f,
            g_telemPkt.cell3_mv / 1000.0f);

        // Currents
        display.setCursor(0, 24);
        display.printf("CUR: L:%2.1fA  R:%2.1fA",
            g_telemPkt.left_current_ma / 1000.0f,
            g_telemPkt.right_current_ma / 1000.0f);

        // Temperatures
        display.setCursor(0, 35);
        display.printf("TMP: B:%2dC  M:%2d/%2dC",
            g_telemPkt.temp_battery_c,
            g_telemPkt.temp_left_drv_c,
            g_telemPkt.temp_right_drv_c);

        display.drawLine(0, 46, 128, 46, SSD1306_WHITE);

        // Active Speed & Link State
        display.setCursor(0, 52);
        bool linkAlive = (millis() - g_lastTelemTime < 300);
        display.printf("RPM:%3d  LINK:%s",
            g_ctrlPkt.target_rpm,
            linkAlive ? "OK" : "LOST");

    } else if (g_uiPage == 1) {
        // --- PAGE 1: SPEED TUNING MENU ---
        display.setTextSize(1);
        display.setCursor(18, 2);
        display.print("== SPEED TUNING ==");
        display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

        display.setCursor(10, 18);
        display.printf("%s Base:   %3d RPM", (g_editCursor == 0 ? ">" : " "), g_settings.speed_base);

        display.setCursor(10, 30);
        display.printf("%s Trig 1: %3d RPM", (g_editCursor == 1 ? ">" : " "), g_settings.speed_trig1);

        display.setCursor(10, 42);
        display.printf("%s Trig 2: %3d RPM", (g_editCursor == 2 ? ">" : " "), g_settings.speed_trig2);

        display.setCursor(12, 55);
        display.print("Click knob to save");
    }

    display.display();
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);

    // 1. Configure Button Pins with Internal Pull-Ups
    pinMode(BTN_UPPER_LEFT, INPUT_PULLUP);
    pinMode(BTN_LOWER_LEFT, INPUT_PULLUP);
    pinMode(BTN_UPPER_RIGHT, INPUT_PULLUP);
    pinMode(BTN_LOWER_RIGHT, INPUT_PULLUP);
    pinMode(BTN_TRIGGER_1, INPUT_PULLUP);
    pinMode(BTN_TRIGGER_2, INPUT_PULLUP);

    // 2. Configure Haptic Motor Output
    pinMode(PIN_HAPTIC, OUTPUT);
    digitalWrite(PIN_HAPTIC, LOW);

    // 3. Configure Rotary Encoder Pins
    pinMode(PIN_ENCODER_CLK, INPUT_PULLUP);
    pinMode(PIN_ENCODER_DT, INPUT_PULLUP);
    pinMode(PIN_ENCODER_SW, INPUT_PULLUP);

    // 4. Initialize OLED Display (I2C)
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 OLED failed to start!");
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 25);
    display.print("ROBO RACE REMOTE");
    display.display();

    // 5. Load User Settings from NVS Flash
    prefs.begin("racebot", false);
    g_settings.speed_base  = prefs.getUShort("spd_base", 400);
    g_settings.speed_trig1 = prefs.getUShort("spd_t1", 500);
    g_settings.speed_trig2 = prefs.getUShort("spd_t2", 600);

    // 6. Initialize ESP-NOW
    WiFi.mode(WIFI_STA);
    esp_wifi_set_ps(WIFI_PS_NONE); // Full power, zero sleep
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Init Error");
        return;
    }

    // Register Broadcast Peer initially
    esp_now_peer_info_t peerInfo = {};
    memset(&peerInfo, 0, sizeof(peerInfo));
    memset(peerInfo.peer_addr, 0xFF, 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    esp_now_register_recv_cb(OnDataRecv);

    triggerHaptic(80); // Quick boot vibration
    delay(500);
}

// ============================================================================
// MAIN LOOP: High-Speed 200 Hz Control & Background UI
// ============================================================================
void loop() {
    static uint32_t lastControlTime = 0;
    static uint32_t lastOledTime = 0;
    uint32_t now = millis();

    // 1. High-Speed 200 Hz Control Loop (Every 5 ms)
    if (now - lastControlTime >= 5) {
        sampleButtonsAndSend();
        lastControlTime = now;
    }

    // 2. Sample Rotary Encoder Inputs
    handleEncoder();

    // 3. Update Haptic Motor Timer
    updateHaptic();

    // 4. Update OLED Display at 10 Hz (Every 100 ms)
    if (now - lastOledTime >= 100) {
        updateOledDisplay();
        lastOledTime = now;
    }
}
