#ifndef __RC_H
#define __RC_H

#include "stm32f4xx_hal.h"

#define RC_PULSE_MIN        1000
#define RC_PULSE_MAX        2000
#define RC_PULSE_CENTER     1500
#define RC_DEADZONE         40
#define RC_VALID_MIN        800
#define RC_VALID_MAX        2200

/* Ch1(转向): PB6, TIM4_CH1 */
#define RC_CH1_PORT         GPIOB
#define RC_CH1_PIN          GPIO_PIN_6

/* Ch2(油门): PA1, TIM5_CH2 */
#define RC_CH2_PORT         GPIOA
#define RC_CH2_PIN          GPIO_PIN_1

typedef struct {
    volatile uint32_t ch1;
    volatile uint32_t ch2;
    volatile uint8_t  ch1_valid;
    volatile uint8_t  ch2_valid;
} RC_Data;

extern RC_Data rc_data;
void RC_Init(void);

#endif
