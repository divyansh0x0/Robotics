#include <Arduino.h>
#include "ESPHotspot.h"
#include "L298NController.h"

#define SSID "ESP8266 2W 7"
#define PASSWORD "12345678"
#define ValueCount 2

#define ENA D1
#define ENB D2
#define IN1 D3
#define IN2 D4
#define IN3 D5
#define IN4 D6

float realBuffer[ValueCount];
float tempBuffer[ValueCount];
Robo::ESPHotspot Hotspot;
Robo::L298NController MotorController;

void setup() {
    memset(realBuffer, 0, sizeof(float) * ValueCount);
    memset(tempBuffer, 0, sizeof(float) * ValueCount);

    Serial.begin(115200);

    MotorController.init(ENA, ENB, IN1, IN2, IN3, IN4);
    Hotspot.start(SSID, PASSWORD);

    Serial.println("Started Hotspot");
}

void loop() {
    Hotspot.update();

    if (Hotspot.connected()) {
        if (Hotspot.tryReadExact(realBuffer, ValueCount)) {
            Serial.print("x:");
            Serial.print(realBuffer[0]);
            Serial.print(",");
            Serial.print("y:");
            Serial.println(realBuffer[1]);
            MotorController.update(realBuffer[0], realBuffer[1]);
        }
    }
    else {
            MotorController.update(0,0);

    }
    yield();
}
