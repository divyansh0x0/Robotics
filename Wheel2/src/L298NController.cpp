//
// Created by divyansh on 3/4/26.
//

#include "L298NController.h"
#include <Arduino.h>

#define MAX_PWM 255

static int mapRange(double v, float in_min, float in_max, int out_min, int out_max) {
    if (v < in_min) v = in_min;
    if (v > in_max) v = in_max;

    return round(v - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
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

    void L298NController::setCorrection(int left_motor_correction, int right_motor_correction) {
        m_left_motor_status.correction = left_motor_correction;
        m_right_motor_status.correction = right_motor_correction;
    }

    void L298NController::setLeftMotor(const int speed, const MotorDirection direction) {
        m_left_motor_status.direction = direction;
        m_left_motor_status.speed = std::clamp(speed, 0, MAX_PWM);
    }

    void L298NController::setRightMotor(const  int speed, const MotorDirection direction) {
        m_right_motor_status.direction = direction;
        m_right_motor_status.speed = std::clamp(speed, 0, MAX_PWM);
    }

    void L298NController::update() const {
        setMotorSignals(m_left_motor_pins, m_left_motor_status);
        setMotorSignals(m_right_motor_pins, m_right_motor_status);

        // Serial.print(m_left_motor_status.speed);
        // Serial.print(" | ");
        // Serial.println(m_right_motor_status.speed);

        Serial.print("left:");
        Serial.print(m_left_motor_status.speed);
        Serial.print(",");
        Serial.print("right:");
        Serial.println(m_right_motor_status.speed);
    }

    // X = xcos - ysin; Y = xsin - ycos

    void Robo::L298NController::update(float x, float y) {
        if (abs(x) <= 0.05 && abs(y) <= 0.05) {
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


        if (x < 0) {
            v1 = mag;
            v2 = mag * t;
        }
        else {

            v1 = mag * t;
            v2 = mag;
        }
        setLeftMotor((int)round((v1 * MAX_PWM)), getDirection(y));
        setRightMotor(( int)round((v2 * MAX_PWM)), getDirection(y));
        update();
    }
}
