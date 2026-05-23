//
// Created by divyansh on 3/4/26.
//

#ifndef WHEEL2_TB6612FNGCONTROLLER_H
#define WHEEL2_TB6612FNGCONTROLLER_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

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
        TIM_HandleTypeDef* htim;
        uint32_t           channel;
        GPIO_TypeDef*      in1_port;
        uint16_t           in1_pin;
        GPIO_TypeDef*      in2_port;
        uint16_t           in2_pin;
    };

    class TB6612FNGController {
        MotorStatus m_left_motor_status{0, 1.0f, MotorDirection::STOP};
        MotorStatus m_right_motor_status{0, 1.0f, MotorDirection::STOP};
        MotorPins   m_left_motor_pins{};
        MotorPins   m_right_motor_pins{};
        GPIO_TypeDef* m_stdby_port;
        uint16_t      m_stdby_pin;

        void update() const;
        void setLeftMotor(int speed, MotorDirection direction);
        void setRightMotor(int speed, MotorDirection direction);

    public:
        void init(MotorPins left_pins, MotorPins right_pins,
                  GPIO_TypeDef* stdby_port, uint16_t stdby_pin);
        void setCorrection(float left_motor_correction, float right_motor_correction);
        void update(float x, float y);
        int getLeftSpeed() const { return m_left_motor_status.speed; }
        int getRightSpeed() const { return m_right_motor_status.speed; }
    };

} // namespace Robo

#endif // WHEEL2_TB6612FNGCONTROLLER_H
