# Hand Gestures Controlled LED Matrix System

A real-time, webcam-based hand-gesture controller built on the **ESP32**, driving **5 individual PWM LEDs** and an **8×8 MAX7219 dot-matrix display** via **Direct USB Serial** or **Bluetooth Classic (SPP)** directly from a web browser.

---

## ✨ Features & Operating Modes

The system supports **4 real-time interactive modes**, seamlessly synchronized between the web dashboard, the 5 hardware LEDs, and the 8×8 LED matrix:

| Mode | Name | Hand Gesture | 5-LED Output | 8×8 Matrix Output | Serial Command |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Mode 1** | **Palm Switch** | Open Palm / Closed Fist | All 5 LEDs `ON` or `OFF` | Displays **`✓` (Tick)** or **`✕` (Cross)** | `MODE1:ON` / `MODE1:OFF` |
| **Mode 2** | **Pinch Dimmer** | Thumb–Index Pinch Distance | Smooth 5 kHz PWM Dimming (`0–255`) | Displays brightness percentage (`00–99`) on a `4×8 + 4×8` split screen | `MODE2:<0-255>` |
| **Mode 3** | **Finger Counter** | Total Raised Fingers (`0–5`) | Lights `N` LEDs sequentially in order | Displays crisp digit **`0` through `5`** | `MODE3:<0-5>` |
| **Mode 4** | **Binary Counter** | Individual Finger States (`Thumb` → `Pinky`) | Lights only the specific LEDs matching raised fingers | Converts 5-bit binary (`00000–11111`) and displays decimal **`00–31`** | `MODE4:<5-bit>` |

### Mode 4 Bit Mapping (Thumb = MSB, Pinky = LSB)
* **Bit 4 (Leftmost):** Thumb → `GPIO 13` (LED 1, weight `16`)
* **Bit 3:** Index → `GPIO 14` (LED 2, weight `8`)
* **Bit 2:** Middle → `GPIO 27` (LED 3, weight `4`)
* **Bit 1:** Ring → `GPIO 26` (LED 4, weight `2`)
* **Bit 0 (Rightmost):** Pinky → `GPIO 25` (LED 5, weight `1`)

> **Example:** Raising Thumb + Middle + Pinky sends `MODE4:10101`, turns on LEDs `1, 3, 5`, and displays **`21`** on the 8×8 matrix.

---

## 🔌 Hardware Wiring

### 1. 5-LED Array (PWM)
Connect each LED in series with a **220 Ω – 330 Ω** current-limiting resistor to the following ESP32 GPIOs (boot-strapping pins are avoided):

| LED Channel | Finger Mapping (Mode 4) | ESP32 GPIO |
| :--- | :--- | :--- |
| **LED 1** | Thumb | `GPIO 13` |
| **LED 2** | Index | `GPIO 14` |
| **LED 3** | Middle | `GPIO 27` |
| **LED 4** | Ring | `GPIO 26` |
| **LED 5** | Pinky | `GPIO 25` |

### 2. 8×8 MAX7219 LED Matrix (Hardware VSPI)

| MAX7219 Pin | ESP32 Pin | Description |
| :--- | :--- | :--- |
| **VCC** | `5V (VIN)` | Power Supply |
| **GND** | `GND` | Common Ground |
| **DIN** | `GPIO 23` | VSPI MOSI |
| **CS (LOAD)** | `GPIO 5` | Chip Select |
| **CLK** | `GPIO 18` | VSPI SCK |

---

## 🛠️ Firmware Setup (PlatformIO / Arduino Core 3.x)

1. **Install Dependencies:** Add the `MD_MAX72XX` library to your `platformio.ini`:
   ```ini
   [env:esp32dev]
   platform = espressif32
   board = esp32dev
   framework = arduino
   monitor_speed = 115200
   lib_deps =
       majicdesigns/MD_MAX72XX@^3.5.1
   ```
2. **Flash the ESP32:** Upload `src/main.cpp` to your ESP32 WROOM-32 DevKit.
3. *(Optional)* If symbols appear rotated or mirrored on your specific MAX7219 module, change `#define HARDWARE_TYPE MD_MAX72XX::FC16_HW` in `main.cpp` to `GENERIC_HW`, `PAROLA_HW`, or `ICSTATION_HW`.

---

## 💻 Connecting from the Web Interface

Open the web dashboard in **Google Chrome** or **Microsoft Edge** (Web Serial API supported) and connect using either option:

* **Option A — Direct USB Serial:**
  1. Connect the ESP32 to your laptop via USB.
  2. Click **Connect** on the webpage and select your ESP32's USB COM port (`115200` baud).
* **Option B — Wireless Bluetooth Classic:**
  1. Open your laptop's **Bluetooth Settings** and pair with **`GestureLED-BT`**.
  2. Click **Connect** on the webpage and select the newly created **Standard Serial over Bluetooth link (COMx)** port.

---
