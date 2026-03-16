#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <algorithm>

// --- ESPHotspot.h ---
#ifndef ESP_HOTSPOT_H
#define ESP_HOTSPOT_H

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

        void updateLED() const;

        bool connected() const;
        bool tryReadExact(float* buffer, int size);

    private:
        WiFiServer m_server;
        WiFiClient m_client;
        WiFiState m_state;

    };
}

#endif


// --- L298NController.h ---
#ifndef WHEEL2_L298NCONTROLLER_H
#define WHEEL2_L298NCONTROLLER_H

namespace Robo {

    enum class MotorDirection {
        BACKWARD,
        FORWARD,
        STOP
    };
    struct MotorStatus {
        unsigned int speed;
        unsigned int correction;
        MotorDirection direction;
    };

    struct L298NPins {
        char PWM;
        char IN1;
        char IN2;
    };

    class L298NController {
        MotorStatus m_left_motor_status{0, 0, MotorDirection::STOP};
        MotorStatus m_right_motor_status{0, 0,MotorDirection::STOP};
        L298NPins m_left_motor_pins{};
        L298NPins m_right_motor_pins{};
        void update() const;

        void setLeftMotor(unsigned int speed, MotorDirection direction);

        void setRightMotor(unsigned int speed, MotorDirection direction);
    public:
        void init(char ENA, char ENB, char IN1, char IN2, char IN3, char IN4);
        void setCorrection(unsigned int left_motor_correction, unsigned int right_motor_correction);
        void update(float x, float y);
    };
}
#endif //WHEEL2_L298NCONTROLLER_H


// --- ESPHotspot.cpp ---

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


// --- L298NController.cpp ---

#define MAX_PWM 600

static int mapRange(float v, float in_min, float in_max, int out_min, int out_max) {
    if (v < in_min) v = in_min;
    if (v > in_max) v = in_max;

    return (v - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static Robo::MotorDirection getDirection(float val) {
    return val >= 0
               ? Robo::MotorDirection::FORWARD
               : Robo::MotorDirection::BACKWARD;
}

static float applyDeadzone(float v, float deadzone = 0.08f) {
    if (fabs(v) < deadzone)
        return 0.0f;

    // Rescale so output still reaches full range
    return (v - copysign(deadzone, v)) / (1.0f - deadzone);
}

static float expo(float v, float exponent = 2.0f) {
    return copysign(pow(fabs(v), exponent), v);
}

static void setMotorSignals(Robo::L298NPins motor_pins, Robo::MotorStatus status) {
    switch (status.direction) {
        case Robo::MotorDirection::STOP:
            digitalWrite(motor_pins.IN1, LOW);
            digitalWrite(motor_pins.IN2, LOW);
            break;
        case Robo::MotorDirection::BACKWARD:
            digitalWrite(motor_pins.IN1, LOW);
            digitalWrite(motor_pins.IN2, HIGH);
            break;
        case Robo::MotorDirection::FORWARD:
            digitalWrite(motor_pins.IN1, HIGH);
            digitalWrite(motor_pins.IN2, LOW);
            break;
    }

    analogWrite(motor_pins.PWM, status.speed);
}

namespace Robo {
    void L298NController::init(const char ENA, const char ENB, const char IN1, const char IN2, const char IN3,
                               const char IN4) {
        m_left_motor_pins.PWM = ENA;
        m_left_motor_pins.IN1 = IN1;
        m_left_motor_pins.IN2 = IN2;

        m_right_motor_pins.PWM = ENB;
        m_right_motor_pins.IN1 = IN3;
        m_right_motor_pins.IN2 = IN4;


        m_left_motor_status.direction = MotorDirection::STOP;
        m_left_motor_status.speed = 0;
        m_right_motor_status.direction = MotorDirection::STOP;
        m_right_motor_status.speed = 0;

        pinMode(ENA, OUTPUT);
        pinMode(ENB, OUTPUT);
        pinMode(IN1, OUTPUT);
        pinMode(IN2, OUTPUT);
        pinMode(IN3, OUTPUT);
        pinMode(IN4, OUTPUT);
    }

    void L298NController::setCorrection(unsigned int left_motor_correction, unsigned int right_motor_correction) {
        m_left_motor_status.correction = left_motor_correction;
        m_right_motor_status.correction = right_motor_correction;
    }

    void L298NController::setLeftMotor(const unsigned int speed, const MotorDirection direction) {
        m_left_motor_status.direction = direction;
        m_left_motor_status.speed = std::clamp(speed - m_left_motor_status.correction, 0u, 1023u);
    }

    void L298NController::setRightMotor(const unsigned int speed, const MotorDirection direction) {
        m_right_motor_status.direction = direction;
        m_right_motor_status.speed = std::clamp(speed - m_right_motor_status.correction, 0u, 1023u);
    }

    void L298NController::update() const {
        setMotorSignals(m_left_motor_pins, m_left_motor_status);
        setMotorSignals(m_right_motor_pins, m_right_motor_status);

        // Serial.print(m_left_motor_status.speed);
        // Serial.print(" | ");
        // Serial.println(m_right_motor_status.speed);
    }

    // X = xcos - ysin; Y = xsin - ycos

    void Robo::L298NController::update(float x, float y) {
        if (x == 0 && y == 0) {
            setLeftMotor(0, MotorDirection::STOP);
            setRightMotor(0, MotorDirection::STOP);
            update();
            return;
        }
        const auto theta = atan2(abs(y),abs(x));
        const auto mag = sqrt(x*x + y*y);
        const auto t = theta/PI * 2;
        double v1;
        double v2;
        if (x > 0) {
            v1 = mag;
            v2 = mag * t;
        }
        else {

            v1 = mag * t;
            v2 = mag;
        }
        setLeftMotor((unsigned int)round((v1 * 1023)), getDirection(y));
        setRightMotor((unsigned int)round((v2 * 1023)), getDirection(y));
        update();
    }
}


// --- main.cpp ---

#define SSID "ESP8266 2W 8"
#define PASSWORD "12345678"
#define ValueCount 2

#define ENA D1
#define ENB D2
#define IN1 D3
#define IN2 D4
#define IN3 D5
#define IN4 D6

float realBuffer[ValueCount];
float tempBuffer[ValueCount];
Robo::ESPHotspot Hotspot;
Robo::L298NController MotorController;

void setup() {
    memset(realBuffer, 0, sizeof(float) * ValueCount);
    memset(tempBuffer, 0, sizeof(float) * ValueCount);

    Serial.begin(115200);

    MotorController.init(ENA, ENB, IN1, IN2, IN3, IN4);
    Hotspot.start(SSID, PASSWORD);

    Serial.println("Started Hotspot");
}

void loop() {
    Hotspot.update();

    if (Hotspot.connected()) {
        if (Hotspot.tryReadExact(realBuffer, ValueCount)) {

            MotorController.update(realBuffer[0], realBuffer[1]);
        }
    }
    else {
            MotorController.update(0,0);

    }
    yield();
}
