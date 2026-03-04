#include "ESPHotspot.h"
#include <Arduino.h>
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
    void ESPHotspot::updateLED() {
        static uint32_t lastToggle = 0;
        uint32_t now = millis();

        switch (m_state) {
            case WiFiState::WAITING:
                if (now - lastToggle > 1000) { // slow blink
                    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
                    lastToggle = now;
                }
                break;

            case WiFiState::CONNECTED:
                digitalWrite(LED_BUILTIN, LOW);
                break;
        }
    }
    bool ESPHotspot::connected() {
        return m_client && m_client.connected();
    }

    bool ESPHotspot::tryReadExact(uint8_t* buffer, size_t size) {

        if (!m_client || !m_client.connected())
            return false;

        // If full packet not available, return immediately
        if (m_client.available() < size)
            return false;

        size_t received = 0;

        while (received < size) {
            int len = m_client.read(buffer + received, size - received);
            if (len <= 0)
                return false;
            received += len;
        }

        return true;
    }
}
