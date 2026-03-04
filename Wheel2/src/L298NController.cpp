//
// Created by divyansh on 3/4/26.
//

#include "L298NController.h"
#include <Arduino.h>

static Robo::MotorDirection getDirection(float val) {
    return val > 0
               ? Robo::MotorDirection::FORWARD
               : (val == 0 ? Robo::MotorDirection::STOP : Robo::MotorDirection::BACKWARD);
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
    }

    void L298NController::update(const float x, const float y) {
        const float xFiltered = expo(applyDeadzone(x));
        const float yFiltered = expo(applyDeadzone(y));
        float leftSpeedFraction = yFiltered - xFiltered;
        float rightSpeedFraction = yFiltered + xFiltered;
        const float maxMag = fmax(fabs(leftSpeedFraction), fabs(rightSpeedFraction));
        if (maxMag > 1.0) {
            leftSpeedFraction /= maxMag;
            rightSpeedFraction /= maxMag;
        }


        const auto leftSpeed = static_cast<unsigned int>(abs(leftSpeedFraction * 1023));
        const auto rightSpeed = static_cast<unsigned int>(abs(rightSpeedFraction * 1023));
        setLeftMotor(leftSpeed, getDirection(leftSpeedFraction));
        setRightMotor(rightSpeed, getDirection(rightSpeedFraction));
        update();
    }
}
