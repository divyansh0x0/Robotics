//
// Created by divyansh on 3/4/26.
//

#ifndef WHEEL2_TB6612FNGCONTROLLER_H
#define WHEEL2_TB6612FNGCONTROLLER_H

#include <stdint.h>

namespace Robo {

    enum class MotorDirection {
        BACKWARD = -1,
        FORWARD  =  1,
        STOP     =  0
    };

    struct MotorStatus {
        int speed;
        float correction;
        MotorDirection direction;
    };

    struct MotorPins {
        uint8_t PWM;
        uint8_t IN1;
        uint8_t IN2;
    };

    class TB6612FNGController {
        MotorStatus m_left_motor_status{0, 1.0f, MotorDirection::STOP};
        MotorStatus m_right_motor_status{0, 1.0f, MotorDirection::STOP};
        MotorPins   m_left_motor_pins{};
        MotorPins   m_right_motor_pins{};
        uint8_t     m_stdby_pin{};

        void update() const;
        void setLeftMotor(int speed, MotorDirection direction);
        void setRightMotor(int speed, MotorDirection direction);

    public:
        void init(uint8_t PWMA, uint8_t PWMB, uint8_t AIN1, uint8_t AIN2,
                  uint8_t BIN1, uint8_t BIN2, uint8_t STDBY);
        void setCorrection(float left_motor_correction, float right_motor_correction);
        void update(float x, float y);
    };

} // namespace Robo

#endif // WHEEL2_TB6612FNGCONTROLLER_H
