#include "../include/ESPHotspot.h"
#include <Arduino.h>

// ── Big-Endian float (Java wire format) → host float ──────────────────────────
static inline float readFloatBE(WiFiClient &client) {
    uint8_t b[4];
    client.readBytes(b, 4);

    // Reverse bytes: Java BE [0,1,2,3] → ESP LE [3,2,1,0]
    uint32_t raw = ((uint32_t)b[0] << 24)
                 | ((uint32_t)b[1] << 16)
                 | ((uint32_t)b[2] <<  8)
                 | ((uint32_t)b[3]);

    float f;
    memcpy(&f, &raw, sizeof(f));   // strict-aliasing safe
    return f;
}

namespace Robo {

// ── Constructor ───────────────────────────────────────────────────────────────
ESPHotspot::ESPHotspot(unsigned int port)
    : m_server(port),
      m_state(HotspotState::WAITING),
      m_lastLedToggle(0)
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // LED off (active-low on most ESP boards)
}

// ── start ─────────────────────────────────────────────────────────────────────
void ESPHotspot::start(const char *ssid, const char *password, int channel) {
    // Disable station mode to avoid scan/reconnect watchdog triggers
    WiFi.mode(WIFI_AP);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);   // prevents modem-sleep resets
    WiFi.setOutputPower(17.0f);           // 17 dBm – stable, not max thermal stress

    // max_connection = 1 keeps memory usage minimal
    WiFi.softAP(ssid, password, channel, /*hidden=*/0, /*max_conn=*/1);

    m_server.begin();
    m_server.setNoDelay(true);            // reduces latency, avoids Nagle stalls

}

// ── update (call every loop iteration) ───────────────────────────────────────
void ESPHotspot::update() {
    // Feed the software watchdog – critical if update() is called from a
    // tight loop that doesn't reach delay() or yield() on its own.
    yield();

    switch (m_state) {

        case HotspotState::WAITING: {
            WiFiClient candidate = m_server.accept();
            if (candidate) {
                // Reject if somehow a stale client is still lingering
                if (m_client) {
                    m_client.stop();
                }
                m_client = candidate;
                m_client.setNoDelay(true);
                m_state = HotspotState::CONNECTED;
            }
            break;
        }

        case HotspotState::CONNECTED: {
            if (!m_client || !m_client.connected()) {
                handleDisconnect();
            }
            break;
        }
    }

    updateLED();
}

// ── handleDisconnect ──────────────────────────────────────────────────────────
void ESPHotspot::handleDisconnect() {
    m_client.stop();
    m_state = HotspotState::WAITING;
}

// ── updateLED (non-blocking blink, instance-safe) ────────────────────────────
void ESPHotspot::updateLED() {
    const uint32_t interval = (m_state == HotspotState::WAITING) ? 250u : 1000u;
    const uint32_t now = millis();

    if (now - m_lastLedToggle >= interval) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        m_lastLedToggle = now;
    }
}

// ── connected ─────────────────────────────────────────────────────────────────
bool ESPHotspot::connected() const {
    return m_state == HotspotState::CONNECTED;
}

// ── tryReadExact ──────────────────────────────────────────────────────────────
bool ESPHotspot::tryReadExact(float *buffer, int count) {
    if (!m_client || !m_client.connected()) {
        return false;
    }

    // Bug fix: compare bytes available against bytes needed (count * 4),
    // not against the number of floats.
    const int bytesNeeded = count * static_cast<int>(sizeof(float));
    if (m_client.available() < bytesNeeded) {
        return false;
    }

    for (int i = 0; i < count; ++i) {
        buffer[i] = readFloatBE(m_client);
    }
    return true;
}

} // namespace Robo