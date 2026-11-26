#define WOKWI_SIMULATION // Comment this line out on real ESP32 hardware to enable Bluetooth Classic

#include <Arduino.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#ifndef WOKWI_SIMULATION
#include <BluetoothSerial.h>
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to enable it
#endif
BluetoothSerial SerialBT;
const char* BT_DEVICE_NAME = "GestureLED-BT";
#endif

// ============================================================================
// 5-LED HARDWARE PIN DEFINITIONS & PWM CONFIGURATION
// ============================================================================
const uint8_t LED_PINS[] = {13, 14, 27, 26, 25};
const size_t NUM_LEDS = sizeof(LED_PINS) / sizeof(LED_PINS[0]);

const uint32_t PWM_FREQUENCY = 5000;
const uint8_t  PWM_RESOLUTION = 8;

// ============================================================================
// 8x8 LED MATRIX CONFIGURATION (MAX7219)
// ============================================================================
#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
#define MAX_DEVICES   1
#define CS_PIN        5   // VSPI CS (CLK = 18, DIN = 23)

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// Custom 8x8 Bitmaps (Each byte represents 1 row of 8 pixels)
const uint8_t TICK_BITMAP[8] = {
    0b00000001,
    0b00000011,
    0b00000110,
    0b10001100,
    0b11011000,
    0b01110000,
    0b00100000,
    0b00000000
};

const uint8_t CROSS_BITMAP[8] = {
    0b10000001,
    0b01000010,
    0b00100100,
    0b00011000,
    0b00011000,
    0b00100100,
    0b01000010,
    0b10000001
};

const uint8_t DIGITS_BITMAP[6][8] = {
    // 0
    {0b00111100, 0b01100110, 0b01101110, 0b01110110, 0b01100110, 0b01100110, 0b00111100, 0b00000000},
    // 1
    {0b00011000, 0b00111000, 0b00011000, 0b00011000, 0b00011000, 0b00011000, 0b01111110, 0b00000000},
    // 2
    {0b00111100, 0b01100110, 0b00000110, 0b00011100, 0b00110000, 0b01100000, 0b01111110, 0b00000000},
    // 3
    {0b00111100, 0b01100110, 0b00000110, 0b00011100, 0b00000110, 0b01100110, 0b00111100, 0b00000000},
    // 4
    {0b00001110, 0b00011110, 0b00110110, 0b01100110, 0b01111111, 0b00000110, 0b00000110, 0b00000000},
    // 5
    {0b01111110, 0b01100000, 0b01111100, 0b00000110, 0b00000110, 0b01100110, 0b00111100, 0b00000000}
};

// Compact 3x7 pixel digits (0-9) stored in lower 4 bits for 4x8 + 4x8 split display
const uint8_t SMALL_DIGITS[10][8] = {
    {0b0111, 0b0101, 0b0101, 0b0101, 0b0101, 0b0101, 0b0111, 0b0000}, // 0
    {0b0010, 0b0110, 0b0010, 0b0010, 0b0010, 0b0010, 0b0111, 0b0000}, // 1
    {0b0111, 0b0001, 0b0001, 0b0111, 0b0100, 0b0100, 0b0111, 0b0000}, // 2
    {0b0111, 0b0001, 0b0001, 0b0111, 0b0001, 0b0001, 0b0111, 0b0000}, // 3
    {0b0101, 0b0101, 0b0101, 0b0111, 0b0001, 0b0001, 0b0001, 0b0000}, // 4
    {0b0111, 0b0100, 0b0100, 0b0111, 0b0001, 0b0001, 0b0111, 0b0000}, // 5
    {0b0111, 0b0100, 0b0100, 0b0111, 0b0101, 0b0101, 0b0111, 0b0000}, // 6
    {0b0111, 0b0001, 0b0001, 0b0010, 0b0010, 0b0010, 0b0010, 0b0000}, // 7
    {0b0111, 0b0101, 0b0101, 0b0111, 0b0101, 0b0101, 0b0111, 0b0000}, // 8
    {0b0111, 0b0101, 0b0101, 0b0111, 0b0001, 0b0001, 0b0111, 0b0000}  // 9
};

// ============================================================================
// STATE ENUMS & TRACKING
// ============================================================================
enum ModePrefix {
    MODE_NONE = 0,
    MODE_1,
    MODE_2,
    MODE_3,
    MODE_4
};

ModePrefix activeMode = MODE_NONE;

// ============================================================================
// DUAL LOGGING HELPER (USB SERIAL + BLUETOOTH CLASSIC)
// ============================================================================
void logBoth(const String& msg) {
    Serial.println(msg);
#ifndef WOKWI_SIMULATION
    if (SerialBT.hasClient()) {
        SerialBT.println(msg);
    }
#endif
}

// ============================================================================
// MATRIX DISPLAY HELPER FUNCTIONS
// ============================================================================
void clearMatrix() {
    mx.clear();
}

void drawMatrixBitmap(const uint8_t bitmap[8]) {
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    mx.clear();
    for (uint8_t r = 0; r < 8; r++) {
        mx.setRow(0, r, bitmap[r]);
    }
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void drawMatrixTwoDigits(uint8_t value) {
    if (value > 99) {
        value = 99;
    }
    uint8_t tens = value / 10;
    uint8_t ones = value % 10;

    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    mx.clear();
    for (uint8_t r = 0; r < 8; r++) {
        uint8_t combinedRow = (SMALL_DIGITS[tens][r] << 4) | (SMALL_DIGITS[ones][r] & 0x0F);
        mx.setRow(0, r, combinedRow);
    }
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void drawMatrixPercentage(uint8_t duty) {
    uint8_t percent = map(duty, 0, 255, 0, 99);
    uint8_t hwIntensity = map(duty, 0, 255, 2, 15);
    mx.control(MD_MAX72XX::INTENSITY, hwIntensity);
    drawMatrixTwoDigits(percent);
}

// ============================================================================
// LED CONTROL FUNCTIONS (ESP32 Arduino Core 3.x ledc API)
// ============================================================================
void turnOffAllLEDs() {
    for (size_t i = 0; i < NUM_LEDS; i++) {
        ledcWrite(LED_PINS[i], 0);
    }
    clearMatrix();
}

void initHardware() {
    for (size_t i = 0; i < NUM_LEDS; i++) {
        ledcAttach(LED_PINS[i], PWM_FREQUENCY, PWM_RESOLUTION);
        ledcWrite(LED_PINS[i], 0);
    }
    Serial.println("[Hardware] 5 LEDs initialized on GPIO 13, 14, 27, 26, 25");

    mx.begin();
    mx.control(MD_MAX72XX::INTENSITY, 5);
    mx.clear();
    Serial.println("[Hardware] 8x8 MAX7219 Matrix initialized on CS=5, CLK=18, DIN=23");

#ifndef WOKWI_SIMULATION
    if (SerialBT.begin(BT_DEVICE_NAME)) {
        Serial.printf("[Hardware] Bluetooth Classic initialized as \"%s\"\n", BT_DEVICE_NAME);
    } else {
        Serial.println("[Hardware] Bluetooth Classic initialization failed!");
    }
#endif
}

void setAllLEDsBrightness(uint8_t duty) {
    for (size_t i = 0; i < NUM_LEDS; i++) {
        ledcWrite(LED_PINS[i], duty);
    }
}

void setLEDCount(uint8_t count) {
    if (count > NUM_LEDS) {
        count = NUM_LEDS;
    }
    for (size_t i = 0; i < NUM_LEDS; i++) {
        ledcWrite(LED_PINS[i], (i < count) ? 255 : 0);
    }
}

void setLEDBinary(const char* binaryStr) {
    for (size_t i = 0; i < NUM_LEDS; i++) {
        ledcWrite(LED_PINS[i], (binaryStr[i] == '1') ? 255 : 0);
    }
}

// ============================================================================
// COMMAND PARSER & DISPATCHER
// ============================================================================
void processCommand(const char* rawPayload, size_t length) {
    if (length == 0 || length > 64) {
        logBoth("[Command] Rejected message: invalid length (" + String((unsigned int)length) + " bytes)");
        return;
    }

    char buffer[65];
    memcpy(buffer, rawPayload, length);
    buffer[length] = '\0';

    char* colonPos = strchr(buffer, ':');
    if (!colonPos) {
        logBoth("[Command] Rejected malformed message (missing ':'): " + String(buffer));
        return;
    }

    *colonPos = '\0';
    const char* prefix = buffer;
    const char* payload = colonPos + 1;

    ModePrefix incomingMode = MODE_NONE;
    if (strcmp(prefix, "MODE1") == 0) {
        incomingMode = MODE_1;
    } else if (strcmp(prefix, "MODE2") == 0) {
        incomingMode = MODE_2;
    } else if (strcmp(prefix, "MODE3") == 0) {
        incomingMode = MODE_3;
    } else if (strcmp(prefix, "MODE4") == 0) {
        incomingMode = MODE_4;
    } else {
        logBoth("[Command] Rejected message with unknown prefix: " + String(prefix) + ":" + String(payload));
        return;
    }

    if (incomingMode != activeMode) {
        turnOffAllLEDs();
        mx.control(MD_MAX72XX::INTENSITY, 5);
        activeMode = incomingMode;
        logBoth("[Mode] Switched active mode to " + String(prefix) + " (all outputs reset)");
    }

    switch (incomingMode) {
        case MODE_1: {
            if (strcmp(payload, "ON") == 0) {
                setAllLEDsBrightness(255);
                drawMatrixBitmap(TICK_BITMAP);
                logBoth("[Mode 1] Palm OPEN -> All LEDs ON + Matrix TICK");
            } else if (strcmp(payload, "OFF") == 0) {
                for (size_t i = 0; i < NUM_LEDS; i++) ledcWrite(LED_PINS[i], 0);
                drawMatrixBitmap(CROSS_BITMAP);
                logBoth("[Mode 1] Palm CLOSED -> All LEDs OFF + Matrix CROSS");
            } else {
                logBoth("[Command] Rejected MODE1 invalid payload: " + String(payload));
            }
            break;
        }

        case MODE_2: {
            char* endPtr = nullptr;
            long brightnessVal = strtol(payload, &endPtr, 10);
            if (endPtr == payload || *endPtr != '\0' || brightnessVal < 0 || brightnessVal > 255) {
                logBoth("[Command] Rejected MODE2 invalid brightness: " + String(payload));
                return;
            }
            uint8_t duty = static_cast<uint8_t>(brightnessVal);
            setAllLEDsBrightness(duty);
            drawMatrixPercentage(duty);
            break;
        }

        case MODE_3: {
            char* endPtr = nullptr;
            long countVal = strtol(payload, &endPtr, 10);
            if (endPtr == payload || *endPtr != '\0' || countVal < 0 || countVal > 5) {
                logBoth("[Command] Rejected MODE3 invalid count: " + String(payload));
                return;
            }
            uint8_t count = static_cast<uint8_t>(countVal);
            setLEDCount(count);
            drawMatrixBitmap(DIGITS_BITMAP[count]);
            logBoth("[Mode 3] Finger count " + String(count) + " -> Lighting " + String(count) + " LEDs + Matrix Digit '" + String(count) + "'");
            break;
        }

        case MODE_4: {
            if (strlen(payload) != NUM_LEDS) {
                logBoth("[Command] Rejected MODE4 invalid length (expected 5 bits): " + String(payload));
                return;
            }
            uint8_t decimalVal = 0;
            for (size_t i = 0; i < NUM_LEDS; i++) {
                if (payload[i] == '1') {
                    decimalVal = (decimalVal << 1) | 1;
                } else if (payload[i] == '0') {
                    decimalVal = (decimalVal << 1);
                } else {
                    logBoth("[Command] Rejected MODE4 non-binary character in: " + String(payload));
                    return;
                }
            }
            setLEDBinary(payload);
            drawMatrixTwoDigits(decimalVal);
            logBoth("[Mode 4] Binary " + String(payload) + " -> Decimal " + String(decimalVal));
            break;
        }

        default:
            break;
    }
}

// ============================================================================
// ARDUINO SETUP & LOOP
// ============================================================================
void setup() {
    Serial.begin(115200);
    initHardware();

    Serial.println("==================================================");
    Serial.println(" GestureLED + 8x8 Matrix Ready");
    Serial.println(" Protocol: MODE1:ON/OFF | MODE2:0-255 | MODE3:0-5 | MODE4:00000-11111");
    Serial.println("==================================================");
}

void loop() {
    static String usbCmdBuffer = "";
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            usbCmdBuffer.trim();
            if (usbCmdBuffer.length() > 0) {
                processCommand(usbCmdBuffer.c_str(), usbCmdBuffer.length());
                usbCmdBuffer = "";
            }
        } else {
            if (usbCmdBuffer.length() < 64) {
                usbCmdBuffer += c;
            } else {
                usbCmdBuffer = "";
            }
        }
    }

#ifndef WOKWI_SIMULATION
    static String btCmdBuffer = "";
    while (SerialBT.available() > 0) {
        char c = (char)SerialBT.read();
        if (c == '\n' || c == '\r') {
            btCmdBuffer.trim();
            if (btCmdBuffer.length() > 0) {
                processCommand(btCmdBuffer.c_str(), btCmdBuffer.length());
                btCmdBuffer = "";
            }
        } else {
            if (btCmdBuffer.length() < 64) {
                btCmdBuffer += c;
            } else {
                btCmdBuffer = "";
            }
        }
    }
#endif
}
