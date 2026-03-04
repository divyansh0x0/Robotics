#ifndef ESP_HOTSPOT_H
#define ESP_HOTSPOT_H

#include <ESP8266WiFi.h>

namespace Robo {
    enum class WiFiState {
        CONNECTED,
        WAITING,
    };

    class ESPHotspot {
    public:
        explicit ESPHotspot(uint16_t port = 5000);

        void start(const char *ssid, const char *password);

        void update();

        void updateLED();

        bool connected();

        template<typename T>
        bool tryReceive(T *buffer, const size_t count) {
            if (!m_client || !m_client.connected()) return false;
            return tryReadExact(reinterpret_cast<uint8_t *>(buffer),
                             sizeof(T) * count);
        }

    private:
        WiFiServer m_server;
        WiFiClient m_client;
        WiFiState m_state;

        bool tryReadExact(uint8_t *buffer, size_t size);
    };
}

#endif
