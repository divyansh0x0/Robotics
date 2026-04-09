#ifndef ESP_HOTSPOT_H
#define ESP_HOTSPOT_H

#include <ESP8266WiFi.h>

namespace Robo {
    enum class HotspotState {
        CONNECTED,
        WAITING,
    };

    class ESPHotspot {
    public:
        explicit ESPHotspot(uint16_t port = 5000);

        void start(const char *ssid, const char *password, int channel);

        void update();

        void updateLED() const;

        bool connected() const;
        bool tryReadExact(float* buffer, int size);

    private:
        WiFiServer m_server;
        WiFiClient m_client;
        HotspotState m_state;

    };
}

#endif
