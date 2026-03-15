#include "ESPHotspot.h"
#include <Arduino.h>

static float readFloatBE(WiFiClient &client) {
    union {
        float f;
        uint8_t b[4];
    } data;

    // Read 4 bytes and flip order
    client.readBytes(data.b, 4);

    // Swap bytes: Java (BE) [0,1,2,3] -> ESP (LE) [3,2,1,0]
    uint8_t temp;
    temp = data.b[0];
    data.b[0] = data.b[3];
    data.b[3] = temp;
    temp = data.b[1];
    data.b[1] = data.b[2];
    data.b[2] = temp;

    return data.f;
}

namespace Robo {
    ESPHotspot::ESPHotspot(uint16_t port)
        : m_server(port), m_state(WiFiState::WAITING) {
        pinMode(LED_BUILTIN, OUTPUT);
    }

    void ESPHotspot::start(const char *ssid, const char *password) {
        WiFi.softAP(ssid, password);
        m_server.begin();
    }

    void ESPHotspot::update() {
        switch (m_state) {
            case WiFiState::WAITING: {
                if (WiFiClient newClient = m_server.accept()) {
                    m_client = newClient;
                    m_state = WiFiState::CONNECTED;
                    Serial.println("Client connected");
                }
                break;
            }
            case WiFiState::CONNECTED: {
                if (!m_client.connected()) {
                    m_client.stop();
                    m_state = WiFiState::WAITING;
                    Serial.println("Client disconnected");
                }
            }
        }
        updateLED();
    }

    void ESPHotspot::updateLED() const {
        static uint32_t lastToggle = 0;
        uint32_t now = millis();

        switch (m_state) {
            case WiFiState::WAITING:
                // fast blink
                if (now - lastToggle > 250) {
                    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
                    lastToggle = now;
                }
                break;

            case WiFiState::CONNECTED:
                // slow blink
                if (now - lastToggle > 1000) {
                    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
                    lastToggle = now;
                }
                break;
        }
    }

    // Helper to convert Big-Endian (Java) to Little-Endian (ESP8266)


    bool ESPHotspot::connected() const {
        return m_state == WiFiState::CONNECTED;
    }

    bool ESPHotspot::tryReadExact(float *buffer, int size) {
        if (!m_client || !m_client.connected())
            return false;

        // If full packet not available, return immediately
        if (m_client.available() < size)
            return false;

        for (int i = 0; i < size; i++) {
            buffer[i] = readFloatBE(m_client);
        }
        return true;
    }
}
