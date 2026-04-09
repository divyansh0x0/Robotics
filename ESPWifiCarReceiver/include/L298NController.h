//
// Created by divyansh on 3/4/26.
//

#ifndef WHEEL2_L298NCONTROLLER_H
#define WHEEL2_L298NCONTROLLER_H

namespace Robo {

    enum class MotorDirection{
        BACKWARD = -1,
        FORWARD = 1,
        STOP = 0
    };
    struct MotorStatus {
        int speed;
        float correction;
        MotorDirection direction;
    };

    struct L298NPins {
        char PWM;
        char IN1;
        char IN2;
    };

    class L298NController {
        MotorStatus m_left_motor_status{0, 1.0f, MotorDirection::STOP};
        MotorStatus m_right_motor_status{0, 1.0f,MotorDirection::STOP};
        L298NPins m_left_motor_pins{};
        L298NPins m_right_motor_pins{};
        void update() const;

        void setLeftMotor(int speed, MotorDirection direction);

        void setRightMotor( int speed, MotorDirection direction);
    public:
        void init(char ENA, char ENB, char IN1, char IN2, char IN3, char IN4);
        void setCorrection(float left_motor_correction, float right_motor_correction);
        void update(float x, float y);
    };
}
#endif //WHEEL2_L298NCONTROLLER_H
