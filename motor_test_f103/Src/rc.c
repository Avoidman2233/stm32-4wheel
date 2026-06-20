#include "rc.h"

RC_Data rc_data;

static TIM_HandleTypeDef htim4, htim2;
static volatile uint32_t ch1_start_edge, ch2_start_edge;
static volatile uint32_t ch1_last_frame, ch2_last_frame;
static volatile uint8_t ch1_waiting_end, ch2_waiting_end;
static volatile uint8_t ch1_seen_frame, ch2_seen_frame;

static inline uint32_t RC_TimerElapsed(TIM_HandleTypeDef *htim,
                                       uint32_t now, uint32_t start)
{
    if (now >= start) return now - start;
    return (htim->Init.Period - start) + now + 1U;
}

static inline void RC_IC_IRQHandler(TIM_HandleTypeDef *htim, uint32_t flag,
                                     uint32_t channel,
                                     volatile uint32_t *start_edge,
                                     volatile uint32_t *last_frame,
                                     volatile uint8_t *waiting_end,
                                     volatile uint8_t *seen_frame,
                                     volatile uint32_t *pulse,
                                     volatile uint8_t *valid)
{
    if (!__HAL_TIM_GET_FLAG(htim, flag)) return;
    __HAL_TIM_CLEAR_FLAG(htim, flag);
    uint32_t now = HAL_TIM_ReadCapturedValue(htim, channel);

    if (!*waiting_end) {
        uint32_t frame_gap = RC_TimerElapsed(htim, now, *last_frame);
        if (!*seen_frame || frame_gap >= RC_FRAME_MIN_US) {
            *start_edge = now;
            *last_frame = now;
            *seen_frame = 1;
            *waiting_end = 1;
#if RC_SIGNAL_INVERTED
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, channel, TIM_ICPOLARITY_RISING);
#else
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, channel, TIM_ICPOLARITY_FALLING);
#endif
        }
    } else {
        uint32_t width = RC_TimerElapsed(htim, now, *start_edge);
        if (width >= RC_VALID_MIN && width <= RC_VALID_MAX) {
            *pulse = width;
            *valid = 1;
        }
        *waiting_end = 0;
#if RC_SIGNAL_INVERTED
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, channel, TIM_ICPOLARITY_FALLING);
#else
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, channel, TIM_ICPOLARITY_RISING);
#endif
    }
}

void RC_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Mode = GPIO_MODE_AF_INPUT;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    /* CH2 (throttle): PA1 -> TIM2_CH2 */
    gpio.Pin = RC_CH2_PIN;
    HAL_GPIO_Init(RC_CH2_PORT, &gpio);

    /* CH1 (steering): PB6 -> TIM4_CH1 */
    gpio.Pin = RC_CH1_PIN;
    HAL_GPIO_Init(RC_CH1_PORT, &gpio);

    /* TIM4 (16-bit): prescaler 71 -> 72MHz/72 = 1MHz = 1us resolution */
    __HAL_RCC_TIM4_CLK_ENABLE();
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 71;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 0xFFFF;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_IC_Init(&htim4);

    TIM_IC_InitTypeDef ic = {0};
#if RC_SIGNAL_INVERTED
    ic.ICPolarity = TIM_ICPOLARITY_FALLING;
#else
    ic.ICPolarity = TIM_ICPOLARITY_RISING;
#endif
    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic.ICPrescaler = TIM_ICPSC_DIV1;
    ic.ICFilter = 4;
    HAL_TIM_IC_ConfigChannel(&htim4, &ic, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_1);
    HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);

    /* TIM2_CH2: prescaler 71 -> 1us resolution, 16-bit period is enough */
    __HAL_RCC_TIM2_CLK_ENABLE();
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 71;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 0xFFFF;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_IC_Init(&htim2);

    HAL_TIM_IC_ConfigChannel(&htim2, &ic, TIM_CHANNEL_2);
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
    HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    rc_data.ch1 = RC_PULSE_CENTER;
    rc_data.ch2 = RC_PULSE_CENTER;
}

void TIM4_IRQHandler(void)
{
    RC_IC_IRQHandler(&htim4, TIM_FLAG_CC1, TIM_CHANNEL_1,
                     &ch1_start_edge, &ch1_last_frame,
                     &ch1_waiting_end, &ch1_seen_frame,
                     &rc_data.ch1, &rc_data.ch1_valid);
}

void TIM2_IRQHandler(void)
{
    RC_IC_IRQHandler(&htim2, TIM_FLAG_CC2, TIM_CHANNEL_2,
                     &ch2_start_edge, &ch2_last_frame,
                     &ch2_waiting_end, &ch2_seen_frame,
                     &rc_data.ch2, &rc_data.ch2_valid);
}
