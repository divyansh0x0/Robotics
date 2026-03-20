#include <Arduino.h>
#include "ESPWifi.h"

#define SSID "Galaxy A55"
#define PASSWORD "@g@l@xY@55"
#define ValueCount 2
#define LED_PIN D2
#define SERVO_PIN D1

Robo::ESPWifi wifi{8080};
float realBuffer[ValueCount];

// ================= SERVO STATE =================
int currentPulse = 1500; // µs (default 90°)

enum class ServoState {
    LOW_PHASE,
    HIGH_PHASE
};

ServoState servoState = ServoState::LOW_PHASE;

unsigned long lastMicros = 0;

// ================= NON-BLOCKING SERVO =================
void updateServo() {
    unsigned long now = micros();

    switch (servoState) {
        case ServoState::LOW_PHASE:
            // Wait until next 20ms cycle
            if (now - lastMicros >= 20000) {
                lastMicros = now;
                digitalWrite(SERVO_PIN, HIGH);
                servoState = ServoState::HIGH_PHASE;
            }
            break;

        case ServoState::HIGH_PHASE:
            // Keep HIGH for pulse width
            if (now - lastMicros >= (unsigned long) currentPulse) {
                digitalWrite(SERVO_PIN, LOW);
                servoState = ServoState::LOW_PHASE;
            }
            break;
    }
}

// Set angle instantly
void setAngle(int angle) {
    currentPulse = map(angle, 0, 180, 500, 2400);
}

// ================= SETUP =================
void setup() {
    memset(realBuffer, 0, sizeof(realBuffer));

    Serial.begin(115200);

    wifi.start(SSID, PASSWORD);
    pinMode(SERVO_PIN, OUTPUT);

    Serial.println("Started");
}

void setLEDState(int state) {
    switch (state) {
        case 0:
            digitalWrite(LED_PIN, LOW);
            break;
        default:
            digitalWrite(LED_PIN, HIGH);
            break;

    }
}

// ================= LOOP =================
void loop() {
    wifi.update();

    if (wifi.connected()) {
        if (wifi.tryReadExact(realBuffer, ValueCount)) {
            int state = (int) round(realBuffer[1]);
            setLEDState(state);

            Serial.print("id:");
            Serial.print(realBuffer[0]);
            Serial.print(", state:");
            Serial.println(state);
        }
    } else {
        setAngle(0);
    }

    // 🔥 Always run servo FSM
    updateServo();

    yield(); // keep WiFi alive
}
