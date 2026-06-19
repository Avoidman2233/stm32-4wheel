#include "rc.h"

RC_Data rc_data;

static TIM_HandleTypeDef htim4, htim5;
static volatile uint32_t ch1_rise, ch2_rise;

static inline void RC_IC_IRQHandler(TIM_HandleTypeDef *htim, uint32_t flag,
                                     uint32_t channel, GPIO_TypeDef *port,
                                     uint16_t pin, volatile uint32_t *rise,
                                     volatile uint32_t *pulse,
                                     volatile uint8_t *valid)
{
    if (!__HAL_TIM_GET_FLAG(htim, flag)) return;
    __HAL_TIM_CLEAR_FLAG(htim, flag);
    uint32_t now = HAL_TIM_ReadCapturedValue(htim, channel);
    if (port->IDR & pin) {
        *rise = now;
    } else {
        uint32_t w = now - *rise;
        if (w >= RC_VALID_MIN && w <= RC_VALID_MAX) {
            *pulse = w;
            *valid = 1;
        }
    }
}

void RC_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    gpio.Pin = RC_CH2_PIN;
    gpio.Alternate = GPIO_AF2_TIM5;
    HAL_GPIO_Init(RC_CH2_PORT, &gpio);

    gpio.Pin = RC_CH1_PIN;
    gpio.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(RC_CH1_PORT, &gpio);

    __HAL_RCC_TIM4_CLK_ENABLE();
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 83;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 0xFFFF; /* TIM4 是 16 位定时器，ARR 最大值 */
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_IC_Init(&htim4);

    TIM_IC_InitTypeDef ic = {0};
    ic.ICPolarity = TIM_ICPOLARITY_RISING;
    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic.ICPrescaler = TIM_ICPSC_DIV1;
    ic.ICFilter = 4;
    HAL_TIM_IC_ConfigChannel(&htim4, &ic, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_1);
    HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);

    __HAL_RCC_TIM5_CLK_ENABLE();
    htim5.Instance = TIM5;
    htim5.Init.Prescaler = 83;
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = 0xFFFFFFFF;
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_IC_Init(&htim5);

    HAL_TIM_IC_ConfigChannel(&htim5, &ic, TIM_CHANNEL_2);
    HAL_TIM_IC_Start_IT(&htim5, TIM_CHANNEL_2);
    HAL_NVIC_SetPriority(TIM5_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM5_IRQn);

    rc_data.ch1 = RC_PULSE_CENTER;
    rc_data.ch2 = RC_PULSE_CENTER;
}

void TIM4_IRQHandler(void)
{
    RC_IC_IRQHandler(&htim4, TIM_FLAG_CC1, TIM_CHANNEL_1,
                     RC_CH1_PORT, RC_CH1_PIN, &ch1_rise,
                     &rc_data.ch1, &rc_data.ch1_valid);
}

void TIM5_IRQHandler(void)
{
    RC_IC_IRQHandler(&htim5, TIM_FLAG_CC2, TIM_CHANNEL_2,
                     RC_CH2_PORT, RC_CH2_PIN, &ch2_rise,
                     &rc_data.ch2, &rc_data.ch2_valid);
}
