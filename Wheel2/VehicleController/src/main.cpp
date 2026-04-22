#include "TB6612FNGController.h"
#include <Arduino.h>

// --- Pin Definitions ---
#define PWMA A1
#define PWMB A2
#define STDBY A3
#define AIN2 A8
#define AIN1 A9
#define BIN1 PA_10
#define BIN2 PA_11

// Pin to toggle UART (Connect to a switch or jumper)
// HIGH = Disable UART (Flash Mode)
// LOW = Enable UART (Normal Run Mode)
#define FLASH_MODE_PIN PB0

// --- Objects & Globals ---
Robo::TB6612FNGController MotorController;
HardwareSerial CustomSerial(PB11, PB10); // RX, TX

#define ValueCount 2
float joystickBuffer[ValueCount];
bool uartEnabled = false;

// LED Blink Logic
static bool ledOn = false;
static uint32_t lastDataTime = 0;
static uint32_t lastToggleTime = 0;
#define LED_ACTIVE_TIMEOUT  500   // ms of silence before "no data" state
#define LED_BLINK_INTERVAL  150   // ms per half-cycle (300ms full blink = ~3Hz)

void updateLED() {
    bool dataActive = (millis() - lastDataTime < LED_ACTIVE_TIMEOUT);

    if (dataActive) {
        // Blink continuously while data is flowing
        if (millis() - lastToggleTime >= LED_BLINK_INTERVAL) {
            ledOn = !ledOn;
            digitalWrite(LED_BUILTIN, ledOn ? LOW : HIGH); // LOW = on for most STM32 boards
            lastToggleTime = millis();
        }
    } else {
        // No data — LED off
        if (ledOn) {
            digitalWrite(LED_BUILTIN, HIGH); // HIGH = off
            ledOn = false;
        }
    }
}

void triggerLED() {
    lastDataTime = millis(); // Just stamp the time; updateLED() drives the blinking
}

/**
 * Manages the UART state based on the FLASH_MODE_PIN.
 * Prevents re-initializing the Serial port every loop.
 */
void manageUARTState() {
    bool shouldDisable = (digitalRead(FLASH_MODE_PIN) == HIGH);

    if (shouldDisable && uartEnabled) {
        // Switch to FLASH MODE: Shut down UART
        CustomSerial.end();
        pinMode(PA9, INPUT);  // Set TX to high impedance
        pinMode(PA10, INPUT); // Set RX to high impedance
        uartEnabled = false;
        Serial.println("UART DISABLED: ESP8266 Flash Mode Active.");
    }
    else if (!shouldDisable && !uartEnabled) {
        // Switch to RUN MODE: Initialize UART
         }
}

void readSerial() {
    if (!uartEnabled) return;

    static uint8_t state = 0;
    static uint8_t buffer[8];
    static uint8_t index = 0;
    static uint32_t lastByteTime = 0;

    while (CustomSerial.available()) {
        if (state != 0 && (millis() - lastByteTime > 50)) {
            state = 0;
            index = 0;
        }
        lastByteTime = millis();

        uint8_t incoming = CustomSerial.read();

        switch (state) {
            case 0:
                if (incoming == 0xAA) state = 1;
                break;
            case 1:
                state = (incoming == 0x55) ? 2 : 0;
                index = 0;
                break;
            case 2:
                buffer[index++] = incoming;
                if (index >= 8) {
                    memcpy(&joystickBuffer[0], &buffer[0], 4);
                    memcpy(&joystickBuffer[1], &buffer[4], 4);
                    state = 0;
                    triggerLED();

                    // Debug prints to PC
                }
                break;
        }
    }
}

// --- Main Program ---

void setup() {
    memset(joystickBuffer, 0, sizeof(joystickBuffer));

    // Initialize USB Debugging
    Serial.begin(115200);

    // Initialize Control Pin
    pinMode(FLASH_MODE_PIN, INPUT_PULLDOWN);

    // Initialize Hardware
    MotorController.init(PWMA, PWMB, AIN1, AIN2, BIN1, BIN2, STDBY);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.println("STM32 Booted. Checking Flash Mode Switch...");
    CustomSerial.begin(115200);
    uartEnabled = true;
    Serial.println("UART ENABLED: Normal Operation.");
}

void loop() {
    // 2. Only process data and move motors if UART is active
    if (uartEnabled) {
        readSerial();
        MotorController.update(joystickBuffer[0], joystickBuffer[1]);
    } else {
        // Safety: Stop motors if we are in flash mode
        MotorController.update(0, 0);
    }

    // 3. Keep LED blinker running
    updateLED();
}