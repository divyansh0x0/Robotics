#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "ESPWifi.h"

// ================= FLOAT READER (UDP) =================
static float readFloatBE_UDP(uint8_t *buf) {
    union {
        float f;
        uint8_t b[4];
    } data;

    data.b[0] = buf[3];
    data.b[1] = buf[2];
    data.b[2] = buf[1];
    data.b[3] = buf[0];

    return data.f;
}

// ================= IMPLEMENTATION =================

namespace Robo {
    ESPWifi::ESPWifi(uint16_t port)
        : m_port(port), m_state(WiFiState::WAITING), m_pendingPacketSize(0) {
        pinMode(LED_BUILTIN, OUTPUT);
    }

    void ESPWifi::start(const char *ssid, const char *password) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);

        Serial.print("Connecting to WiFi");

        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }

        Serial.println("\nConnected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

        // ✅ ONLY UDP
        m_udp.begin(m_port);

        // WiFi connected = ready
        m_state = WiFiState::CONNECTED;
    }

    void ESPWifi::update() {
        if (WiFi.status() != WL_CONNECTED) {
            m_state = WiFiState::WAITING;
            updateLED();
            return;
        }
        m_state = WiFiState::CONNECTED;

        m_pendingPacketSize = m_udp.parsePacket(); // ← store it here

        updateLED();
    }

    void ESPWifi::updateLED() const {
        static uint32_t lastToggle = 0;
        uint32_t now = millis();

        switch (m_state) {
            case WiFiState::WAITING:
                // Fast blink (not connected to WiFi)
                if (now - lastToggle > 250) {
                    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
                    lastToggle = now;
                }
                break;

            case WiFiState::CONNECTED:
                // Slow blink (WiFi OK)
                if (now - lastToggle > 1000) {
                    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
                    lastToggle = now;
                }
                break;
        }
    }

    bool ESPWifi::connected() const {
        // In UDP mode, "connected" = WiFi connected
        return WiFi.status() == WL_CONNECTED;
    }

    bool ESPWifi::tryReadExact(float *buffer, int size) {
        if (m_pendingPacketSize <= 0) return false;

        int packetSize = m_pendingPacketSize;
        m_pendingPacketSize = 0; // ← consume it

        Serial.print("Packet size: ");
        Serial.println(packetSize);

        if (packetSize < size * 4) {
            Serial.println("Packet too small");
            return false;
        }

        uint8_t buf[128];
        int len = m_udp.read(buf, sizeof(buf));
        if (len < size * 4) return false;

        for (int i = 0; i < size; i++) {
            buffer[i] = readFloatBE_UDP(&buf[i * 4]);
        }

        return true;
    }
} // namespace Robo
