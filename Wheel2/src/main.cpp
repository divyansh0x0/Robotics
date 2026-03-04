#include <Arduino.h>
#include "ESPHotspot.h"

#define SSID "ESP8266 Hotspot"
#define PASSWORD "12345678"
#define ValueCount 2

float buffer[ValueCount] = {0.};
Robo::ESPHotspot Hotspot;
void setup() {
    Serial.begin(115200);
    Hotspot.start(SSID, PASSWORD);
    Serial.println("Started Hotspot");
}
void loop() {

    Hotspot.update();

    if (Hotspot.connected()) {

        if (Hotspot.tryReceive<float>(buffer, ValueCount)) {
            Serial.println("Received floats");
        }

    }

    yield();
}