#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f1xx_hal.h"

/* 引脚定义 */
#define MOTOR1_PWM_PORT         GPIOA
#define MOTOR1_PWM_PIN          GPIO_PIN_7
#define MOTOR1_PWM_TIM          TIM3
#define MOTOR1_PWM_CHANNEL      TIM_CHANNEL_2

#define MOTOR1_INA_PORT         GPIOB
#define MOTOR1_INA_PIN          GPIO_PIN_0
#define MOTOR1_INB_PORT         GPIOB
#define MOTOR1_INB_PIN          GPIO_PIN_1

#define MOTOR2_PWM_PORT         GPIOA
#define MOTOR2_PWM_PIN          GPIO_PIN_6
#define MOTOR2_PWM_TIM          TIM3
#define MOTOR2_PWM_CHANNEL      TIM_CHANNEL_1

#define MOTOR2_INA_PORT         GPIOB
#define MOTOR2_INA_PIN          GPIO_PIN_4
#define MOTOR2_INB_PORT         GPIOB
#define MOTOR2_INB_PIN          GPIO_PIN_3

#define MOTOR_PWM_FREQ_HZ       20000
#define MOTOR1_TIM_CLK          72000000UL   /* APB1 timer clk = 72MHz (x2) */
#define MOTOR2_TIM_CLK          72000000UL   /* APB1 timer clk = 72MHz (x2) */
#define MOTOR1_ARR              ((MOTOR1_TIM_CLK / MOTOR_PWM_FREQ_HZ) - 1)
#define MOTOR2_ARR              ((MOTOR2_TIM_CLK / MOTOR_PWM_FREQ_HZ) - 1)
#define MOTOR_DIR_DEADTIME_MS   100

typedef enum { MOTOR_CH1 = 0, MOTOR_CH2 = 1 } MotorChannel;
typedef enum { MOTOR_BRAKE = 0, MOTOR_FWD = 1, MOTOR_REV = 2, MOTOR_COAST = 3 } MotorState;

void Motor_Init(void);
void Motor_SetState(MotorChannel ch, MotorState state);
void Motor_SetSpeed(MotorChannel ch, uint16_t duty);
void Motor_SetSpeedPercent(MotorChannel ch, uint8_t percent);
void Motor_Brake(MotorChannel ch);
void Motor_Forward(MotorChannel ch, uint8_t percent);
void Motor_Reverse(MotorChannel ch, uint8_t percent);
void Motor_Coast(MotorChannel ch);
void Motor_EmergencyStop(void);

#endif
