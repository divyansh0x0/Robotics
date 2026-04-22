#include <Arduino.h>

#include "ESPHotspot.h"
#define SSID "ESP8266 2W 1"
#define PASSWORD "12345678"


#define ValueCount 2
#define CHANNEL 5

float network_buffer[ValueCount];
Robo::ESPHotspot hotspot{5000};

void setup() {
    memset(network_buffer, 0, sizeof(float) * ValueCount);
    hotspot.start(SSID, PASSWORD, CHANNEL);
    Serial.begin(115200);
}

void sendBuffer() {
    uint8_t header[2] = {0xAA, 0x55}; // Start marker

    Serial.write(header, 2);
    Serial.write((uint8_t *) &network_buffer[0], 4);
    Serial.write((uint8_t *) &network_buffer[1], 4);
}

void loop() {
    hotspot.update();


    if (hotspot.connected()) {
        if (hotspot.tryReadExact(network_buffer, ValueCount))
            sendBuffer();
    } else {
        memset(network_buffer, 0.f, sizeof(float) * ValueCount);
        sendBuffer();
    }
}
