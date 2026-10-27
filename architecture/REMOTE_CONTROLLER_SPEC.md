# Remote Controller Specification & Menu Architecture

## 1. Overview & Control Philosophy

The handheld remote is engineered for **instant reflex response** and **on-the-fly tuning**:
* **4 Tactile Direction Buttons:** Physical microswitches trigger tank-drive steering with zero deadband or stick-travel latency.
* **2 Dedicated Speed Triggers:** Index-finger triggers immediately override motor speed without navigating menus.
* **0.96" OLED Display & Rotary Encoder:** A clickable rotary knob enables live telemetry monitoring, speed preset tuning, and battery calibration without needing a PC.
* **Master Architecture:** The remote calculates all speeds, holds all configuration in local NVS flash, and sends raw execution commands to the bot.

---

## 2. 4-Button Tank Drive State Matrix

The remote reads the 4 directional buttons simultaneously (`0` = Pressed, `1` = Released via `INPUT_PULLUP`):

| Upper Left (GPIO 13) | Lower Left (GPIO 12) | Upper Right (GPIO 14) | Lower Right (GPIO 27) | Resulting Robot Motion | Left Motor Direction | Right Motor Direction |
|:---:|:---:|:---:|:---:|---|:---:|:---:|
| **PRESSED** | Released | **PRESSED** | Released | **Straight Forward** | FORWARD | FORWARD |
| Released | **PRESSED** | Released | **PRESSED** | **Straight Reverse** | REVERSE | REVERSE |
| **PRESSED** | Released | Released | **PRESSED** | **360° Clockwise Spin** | FORWARD | REVERSE |
| Released | **PRESSED** | **PRESSED** | Released | **360° Counter-Clockwise Spin** | REVERSE | FORWARD |
| **PRESSED** | Released | Released | Released | **Sweeping Turn Right** | FORWARD | STOP / BRAKE |
| Released | Released | **PRESSED** | Released | **Sweeping Turn Left** | STOP / BRAKE | FORWARD |
| Released | **PRESSED** | Released | Released | **Reverse Pivot Left** | REVERSE | STOP / BRAKE |
| Released | Released | Released | **PRESSED** | **Reverse Pivot Right** | STOP / BRAKE | REVERSE |
| Released | Released | Released | Released | **Active Brake (Full Stop)** | BRAKE (0) | BRAKE (0) |

*(Note: If opposing buttons on the same side are pressed simultaneously, e.g. Upper Left + Lower Left, the firmware automatically interprets it as Neutral/Stop for safety).*

---

## 3. Global Speed Preset Overrides

Speed is a **global scalar** applied to whatever movement is active:

```
               [ User Presses Movement Buttons ]
                              |
              Is TRIGGER 2 (GPIO 33) held?
                  /                  \
                YES                   NO
                /                      \
      [ Apply PRESET 2 ]     Is TRIGGER 1 (GPIO 32) held?
      (Default: 600 RPM)         /                  \
                               YES                   NO
                               /                      \
                     [ Apply PRESET 1 ]      [ Apply BASE SPEED ]
                     (Default: 500 RPM)       (Default: 400 RPM)
```

* **Base Speed:** Active during normal driving (default: 400 RPM / 66% PWM).
* **Trigger 1 Speed:** Activated when left index finger holds Trigger 1 (default: 500 RPM / 83% PWM).
* **Trigger 2 Speed:** Activated when right index finger holds Trigger 2 (default: 600 RPM / 100% Full Punch).

---

## 4. OLED Menu & Rotary Encoder System

The EC11 rotary knob controls navigation across 3 dedicated OLED pages:

### Page 1: Live Racing Dashboard (Default Telemetry View)
```
+--------------------------------+
| BAT: 12.42V [████████]  92%    |
| C1:4.14V  C2:4.14V  C3:4.14V   |
| CUR: L:3.1A  R:3.2A  (6.3A Tot)|
| TMP: B:32°C  M1:44°C  M2:46°C  |
| MODE: [BASE 400]    PING: 2ms  |
+--------------------------------+
```

### Page 2: Speed Presets Tuning Menu
Clicking the encoder enters Edit Mode:
* **Rotate Knob:** Changes the selected speed in steps of 25 RPM (range: 200 to 600 RPM).
* **Click Knob:** Advances to the next preset:
  1. `Base Speed: [ 400 RPM ]`
  2. `Trigger 1 Speed: [ 500 RPM ]`
  3. `Trigger 2 Speed: [ 600 RPM ]`
* **Long Press (1 sec):** Saves all settings to **NVS Flash Memory** and returns to Dashboard.

### Page 3: Battery Multimeter Calibration
* Shows raw ADC readings from the 3 cells.
* Allows fine-tuning voltage multiplier ($0.950$ to $1.050$) to match your physical multimeter reading.
* Click to save permanently into NVS flash memory.

---

## 5. Non-Volatile Flash Storage (NVS) Schema

All settings persist across power cycles using the ESP32 `Preferences` library:

| Key | Type | Default Value | Description |
|---|---|---|---|
| `spd_base` | `uint16_t` | `400` | Default speed (in RPM equivalent) |
| `spd_trig1`| `uint16_t` | `500` | Speed when Trigger 1 is held |
| `spd_trig2`| `uint16_t` | `600` | Speed when Trigger 2 is held |
| `cal_c1`   | `float`    | `1.000` | Calibration factor for Cell 1 |
| `cal_c2`   | `float`    | `1.000` | Calibration factor for Cell 2 |
| `cal_c3`   | `float`    | `1.000` | Calibration factor for Cell 3 |
