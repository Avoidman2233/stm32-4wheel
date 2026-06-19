#include "motor.h"

static TIM_HandleTypeDef htim1, htim3;
static MotorState last_state[2] = {MOTOR_COAST, MOTOR_COAST};
static uint8_t  last_percent[2] = {0, 0};

typedef struct {
    GPIO_TypeDef *ina_port; uint16_t ina_pin;
    GPIO_TypeDef *inb_port; uint16_t inb_pin;
} MotorDirPins;

static const MotorDirPins dir_pins[2] = {
    {MOTOR1_INA_PORT, MOTOR1_INA_PIN, MOTOR1_INB_PORT, MOTOR1_INB_PIN},
    {MOTOR2_INA_PORT, MOTOR2_INA_PIN, MOTOR2_INB_PORT, MOTOR2_INB_PIN}
};

typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    uint32_t arr;
} MotorChInfo;

static const MotorChInfo ch_info[2] = {
    {&htim1, TIM_CHANNEL_1, MOTOR1_ARR},
    {&htim3, TIM_CHANNEL_1, MOTOR2_ARR}
};

static void Motor_TIM_PWM_Init(TIM_TypeDef *instance, TIM_HandleTypeDef *htim,
                                uint32_t arr, GPIO_TypeDef *port, uint16_t pin,
                                uint8_t af)
{
    if (instance == TIM1) __HAL_RCC_TIM1_CLK_ENABLE();
    else                  __HAL_RCC_TIM3_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = pin;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = af;
    HAL_GPIO_Init(port, &gpio);

    htim->Instance = instance;
    htim->Init.Prescaler = 0;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = arr;
    htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(htim);

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 0;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    HAL_TIM_PWM_ConfigChannel(htim, &oc, TIM_CHANNEL_1);
}

static void Motor_GPIO_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    gpio.Pin = MOTOR1_INA_PIN | MOTOR1_INB_PIN;
    HAL_GPIO_Init(MOTOR1_INA_PORT, &gpio);

    gpio.Pin = MOTOR2_INA_PIN | MOTOR2_INB_PIN;
    HAL_GPIO_Init(MOTOR2_INA_PORT, &gpio);

    HAL_GPIO_WritePin(MOTOR1_INA_PORT, MOTOR1_INA_PIN | MOTOR1_INB_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR2_INA_PORT, MOTOR2_INA_PIN | MOTOR2_INB_PIN, GPIO_PIN_SET);
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState = RCC_HSI_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM = 16;
    osc.PLL.PLLN = 336;
    osc.PLL.PLLP = RCC_PLLP_DIV2;
    osc.PLL.PLLQ = 4;
    HAL_RCC_OscConfig(&osc);

    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4;
    clk.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5);
}

void Motor_Init(void)
{
    HAL_Init();
    SystemClock_Config();
    Motor_GPIO_Init();
    Motor_TIM_PWM_Init(TIM1, &htim1, MOTOR1_ARR,
                       MOTOR1_PWM_PORT, MOTOR1_PWM_PIN, GPIO_AF1_TIM1);
    Motor_TIM_PWM_Init(TIM3, &htim3, MOTOR2_ARR,
                       MOTOR2_PWM_PORT, MOTOR2_PWM_PIN, GPIO_AF2_TIM3);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

void Motor_SetSpeed(MotorChannel ch, uint16_t duty)
{
    const MotorChInfo *ci = &ch_info[ch];
    __HAL_TIM_SET_COMPARE(ci->htim, ci->channel, duty);
}

void Motor_SetSpeedPercent(MotorChannel ch, uint8_t percent)
{
    if (percent > 100) percent = 100;
    if (percent == last_percent[ch]) return;
    last_percent[ch] = percent;
    const MotorChInfo *ci = &ch_info[ch];
    Motor_SetSpeed(ch, (uint16_t)((uint32_t)percent * ci->arr / 100));
}

void Motor_SetState(MotorChannel ch, MotorState state)
{
    if (last_state[ch] == state) return;
    const MotorDirPins *p = &dir_pins[ch];
    switch (state) {
    case MOTOR_BRAKE:
        HAL_GPIO_WritePin(p->ina_port, p->ina_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(p->inb_port, p->inb_pin, GPIO_PIN_RESET);
        last_percent[ch] = 0;
        Motor_SetSpeed(ch, 0);
        break;
    case MOTOR_FWD:
        HAL_GPIO_WritePin(p->ina_port, p->ina_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(p->inb_port, p->inb_pin, GPIO_PIN_SET);
        break;
    case MOTOR_REV:
        HAL_GPIO_WritePin(p->ina_port, p->ina_pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(p->inb_port, p->inb_pin, GPIO_PIN_RESET);
        break;
    case MOTOR_COAST:
        HAL_GPIO_WritePin(p->ina_port, p->ina_pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(p->inb_port, p->inb_pin, GPIO_PIN_SET);
        last_percent[ch] = 0;
        Motor_SetSpeed(ch, 0);
        break;
    }
    last_state[ch] = state;
}

void Motor_Brake(MotorChannel ch)  { Motor_SetState(ch, MOTOR_BRAKE); }
void Motor_Coast(MotorChannel ch)  { Motor_SetState(ch, MOTOR_COAST); }

void Motor_Forward(MotorChannel ch, uint8_t percent)
{
    if (last_state[ch] == MOTOR_REV) {
        Motor_SetState(ch, MOTOR_BRAKE);
        HAL_Delay(MOTOR_DIR_DEADTIME_MS);
    }
    Motor_SetState(ch, MOTOR_FWD);
    Motor_SetSpeedPercent(ch, percent);
}

void Motor_Reverse(MotorChannel ch, uint8_t percent)
{
    if (last_state[ch] == MOTOR_FWD) {
        Motor_SetState(ch, MOTOR_BRAKE);
        HAL_Delay(MOTOR_DIR_DEADTIME_MS);
    }
    Motor_SetState(ch, MOTOR_REV);
    Motor_SetSpeedPercent(ch, percent);
}

void Motor_EmergencyStop(void) { Motor_Brake(MOTOR_CH1); Motor_Brake(MOTOR_CH2); }
