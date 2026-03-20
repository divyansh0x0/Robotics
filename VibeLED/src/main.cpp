#include <Arduino.h>
#include "ESPWifi.h"

#define SSID "Galaxy A55"
#define PASSWORD "@g@l@xY@55"
#define ValueCount 2
#define LED_PIN D2
#define ESP_ID 1
Robo::ESPWifi wifi{8080};
float realBuffer[ValueCount];

// ================= SETUP =================
void setup() {
    memset(realBuffer, 0, sizeof(realBuffer));

    Serial.begin(115200);

    wifi.start(SSID, PASSWORD);
    pinMode(LED_PIN, OUTPUT);

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
            // int state = (int) round(realBuffer[1]);
            // setLEDState(state);
            int id = (int) round(realBuffer[0]);
            int state = (int) round(realBuffer[1]);
            Serial.print("id:");
            Serial.print(realBuffer[0]);
            Serial.print(", state:");
            Serial.println(realBuffer[1]);
            if (id == ESP_ID) {
                setLEDState(state);

            }
        }
    } else {
        setLEDState(0);
    }

    yield(); // keep WiFi alive
}
