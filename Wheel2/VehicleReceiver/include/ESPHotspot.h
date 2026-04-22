#pragma once

#include <ESP8266WiFi.h>

namespace Robo {

    enum class HotspotState : uint8_t {
        WAITING,
        CONNECTED
    };

    class ESPHotspot {
    public:
        explicit ESPHotspot(unsigned int port);

        void start(const char *ssid, const char *password, int channel = 1);
        void update();

        [[nodiscard]] bool connected() const;

        // Reads exactly `count` floats (Big-Endian from Java) into buffer.
        // Returns true only if all bytes were available and read successfully.
        bool tryReadExact(float *buffer, int count);

    private:
        WiFiServer  m_server;
        WiFiClient  m_client;
        HotspotState m_state;
        uint32_t    m_lastLedToggle;

        void updateLED();
        void handleDisconnect();
    };

} // namespace Robo