#ifndef __RC_H
#define __RC_H

#include "stm32f1xx_hal.h"

/* R8EF: measured low endpoint can be below nominal 1000us after level shifting. */
#define RC_PULSE_MIN        850
#define RC_PULSE_MAX        2200
#define RC_PULSE_CENTER     1500
#define RC_CH1_CENTER       1575
#define RC_CH2_CENTER       1500
#define RC_CH1_DEADZONE     120
#define RC_CH2_DEADZONE     140
#define RC_VALID_MIN        700
#define RC_VALID_MAX        (RC_PULSE_MAX + 100)
#define RC_FRAME_MIN_US     12000
#define RC_FRAME_MAX_US     28000

/* Set to 1 if the level shifter inverts the R8EF pulse. */
#define RC_SIGNAL_INVERTED  0

/* Ch1(steering): PB6, TIM4_CH1 */
#define RC_CH1_PORT         GPIOB
#define RC_CH1_PIN          GPIO_PIN_6

/* Ch2(throttle): PA1, TIM2_CH2 (F103 has no TIM5) */
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
