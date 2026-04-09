//
// Created by divyansh on 3/4/26.
//

#include "TB6612FNGController.h"
#include <Arduino.h>

#define MAX_PWM 1023
// ===== CONFIG =====
#define DEADZONE 0.05f
#define ACCEL_LIMIT 3.0f   // units per second (tune this)
#define LOOP_TIME 0.02f          // loop time (50Hz)
#define INVERTED
// current state (persist between loops)
float vLeft_cur = 0;
float vRight_cur = 0;

// ===== HELPER =====
float clamp(float x, float a, float b) {
    if (x < a) return a;
    if (x > b) return b;
    return x;
}

float sign(float x) {
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

float limitRate(float target, float current, float rate, float loopTime) {
    float delta = target - current;
    float maxStep = rate * loopTime;

    if (delta > maxStep) delta = maxStep;
    else if (delta < -maxStep) delta = -maxStep;

    return current + delta;
}

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

static void setMotorSignals(Robo::MotorPins motor_pins, Robo::MotorStatus status) {
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
    analogWrite(motor_pins.PWM, static_cast<int>(round(status.correction * status.speed)));
    Serial.printf("speed:");
    Serial.println(round(status.correction * status.speed));
}

namespace Robo {
    void TB6612FNGController::init(const uint8_t PWMA, const uint8_t PWMB, const uint8_t AIN1, const uint8_t AIN2,
                                   const uint8_t BIN1, const uint8_t BIN2, const uint8_t STDBY) {
        m_left_motor_pins.PWM  = PWMA;
        m_left_motor_pins.IN1  = AIN1;
        m_left_motor_pins.IN2  = AIN2;

        m_right_motor_pins.PWM = PWMB;
        m_right_motor_pins.IN1 = BIN1;
        m_right_motor_pins.IN2 = BIN2;

        m_stdby_pin = STDBY;

        m_left_motor_status.direction  = MotorDirection::STOP;
        m_left_motor_status.speed      = 0;
        m_right_motor_status.direction = MotorDirection::STOP;
        m_right_motor_status.speed     = 0;

        pinMode(PWMA,  OUTPUT);
        pinMode(PWMB,  OUTPUT);
        pinMode(AIN1,  OUTPUT);
        pinMode(AIN2,  OUTPUT);
        pinMode(BIN1,  OUTPUT);
        pinMode(BIN2,  OUTPUT);
        pinMode(STDBY, OUTPUT);

        // Drive STDBY HIGH to bring the driver out of standby
        digitalWrite(STDBY, HIGH);

        Serial.begin(115200);
        analogWriteFreq(1000);
        analogWriteRange(MAX_PWM);
    }

    void TB6612FNGController::setCorrection(float left_motor_correction, float right_motor_correction) {
#ifdef INVERTED
        m_left_motor_status.correction = left_motor_correction;
        m_right_motor_status.correction = right_motor_correction;
#else
        m_left_motor_status.correction = right_motor_correction;
        m_right_motor_status.correction = left_motor_correction;
        #endif

    }

    void TB6612FNGController::setLeftMotor(const int speed, const MotorDirection direction) {
        m_left_motor_status.direction = direction;
        m_left_motor_status.speed = clamp(speed, 0, MAX_PWM);
    }

    void TB6612FNGController::setRightMotor(const int speed, const MotorDirection direction) {
        m_right_motor_status.direction = direction;
        m_right_motor_status.speed = clamp(speed, 0, MAX_PWM);
    }

    void TB6612FNGController::update() const {
        setMotorSignals(m_left_motor_pins, m_left_motor_status);
        setMotorSignals(m_right_motor_pins, m_right_motor_status);
        Serial.print("left:");
        Serial.print(static_cast<int>(m_left_motor_status.direction) * m_left_motor_status.speed);
        Serial.print(",");
        Serial.print("right:");
        Serial.println(static_cast<int>(m_right_motor_status.direction) * m_right_motor_status.speed);
    }

    // X = xcos - ysin; Y = xsin - ycos

    void Robo::TB6612FNGController::update(float x, float y) {
        // assume x,y ∈ [-1,1]

        float vL_arc, vR_arc;
        float vL_tank, vR_tank;

        // ===== ARC MODE =====
        float mag = fabs(y);

        float t = fabs(x) / (fabs(x) + fabs(y) + 1e-6f);
        t = pow(t, 1.f); // smoothing

        float vFast = mag;
        float vSlow = mag * (1 - t);

        if (x > 0) {
            vL_arc = vFast;
            vR_arc = vSlow;
        } else {
            vL_arc = vSlow;
            vR_arc = vFast;
        }

        float dir = sign(y);
        vL_arc *= dir;
        vR_arc *= dir;

        // ===== TANK MODE =====
        vL_tank = x;
        vR_tank = -x;

        // ===== BLENDING =====
        float alpha = clamp(fabs(y) * 2.0F, 0.0f, 1.0f);

        float vL_target = alpha * vL_arc + (1 - alpha) * vL_tank;
        float vR_target = alpha * vR_arc + (1 - alpha) * vR_tank;

        // ===== ACCEL LIMITING =====
        // vLeft_cur = limitRate(vL_target, vLeft_cur, ACCEL_LIMIT, LOOP_TIME);
        // vRight_cur = limitRate(vR_target, vRight_cur, ACCEL_LIMIT, LOOP_TIME);
        vLeft_cur = vL_target;
        vRight_cur = vR_target;
        // ===== PWM OUTPUT =====
        int pwmLeft = (int) (vLeft_cur * MAX_PWM);
        int pwmRight = (int) (vRight_cur * MAX_PWM);
#ifdef INVERTED
        setLeftMotor(abs(pwmRight), getDirection(vRight_cur));
        setRightMotor(abs(pwmLeft), getDirection(vLeft_cur));
#else
        setLeftMotor(abs(pwmLeft), getDirection(vLeft_cur));
        setRightMotor(abs(pwmRight), getDirection(vRight_cur));
#endif
        update();
    }
}
