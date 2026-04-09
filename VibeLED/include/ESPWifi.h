#ifndef ESP_WIFI_H
#define ESP_WIFI_H

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>   // <-- add this line
namespace Robo {

    enum class WiFiState {
        WAITING,
        CONNECTED
    };

    class ESPWifi {
    public:
        ESPWifi(uint16_t port);

        void start(const char *ssid, const char *password);
        void update();
        bool connected() const;
        bool tryReadExact(float *buffer, int size);

    private:
        WiFiClient m_client;
        WiFiUDP m_udp;              // ✅ NEW
        uint16_t m_port;            // ✅ NEW
        WiFiState m_state;
        int m_pendingPacketSize;
        void updateLED() const;
    };

} // namespace Robo
#endif
