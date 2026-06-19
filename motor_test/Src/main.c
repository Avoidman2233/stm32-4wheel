#include "motor.h"

int main(void)
{
    Motor_Init();
    HAL_Delay(1000);

    while (1) {
        Motor_Forward(MOTOR_CH1, 40);  Motor_Forward(MOTOR_CH2, 40);
        HAL_Delay(3000);

        Motor_Brake(MOTOR_CH1);         Motor_Brake(MOTOR_CH2);
        HAL_Delay(500);

        Motor_Reverse(MOTOR_CH1, 40);  Motor_Reverse(MOTOR_CH2, 40);
        HAL_Delay(3000);

        Motor_Brake(MOTOR_CH1);         Motor_Brake(MOTOR_CH2);
        HAL_Delay(1000);
    }
}

void SysTick_Handler(void) { HAL_IncTick(); }
