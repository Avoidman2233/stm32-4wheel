#include "motor.h"
#include "rc.h"
#include <stdio.h>
#include <string.h>

/* Heartbeat LED */
#define HEARTBEAT_PORT      GPIOA
#define HEARTBEAT_PIN       GPIO_PIN_4

/* UART debug */
#define DEBUG_UART          USART1
#define DEBUG_BAUDRATE      115200

/* Signal loss timeout (ms) */
#define SIGNAL_LOSS_MS      500
#define CONTROL_PERIOD_MS   20

/* Wiring trims: change these if left/right motors or RC directions are reversed. */
#define LEFT_MOTOR_CHANNEL      MOTOR_CH1
#define RIGHT_MOTOR_CHANNEL     MOTOR_CH2
#define LEFT_MOTOR_REVERSED     0
#define RIGHT_MOTOR_REVERSED    0
#define RC_THROTTLE_REVERSED    1
#define RC_STEERING_REVERSED    0
#define PIVOT_STEERING_ENABLE   0

static UART_HandleTypeDef huart1;
static volatile uint32_t sys_tick_ms = 0;

/* ---- System Clock ---- */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&osc);

    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}

/* ---- UART Debug ---- */
static void Debug_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_9;  /* TX */
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = DEBUG_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

static void debug_print(const char *msg)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), 100);
}

/* ---- Utility ---- */
static int32_t constrain(int32_t val, int32_t min, int32_t max)
{
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

static int32_t abs_i32(int32_t val)
{
    return (val < 0) ? -val : val;
}

static int32_t rc_axis_to_percent(int32_t pulse, int32_t center,
                                  int32_t deadzone, uint8_t reversed)
{
    int32_t offset = pulse - center;
    int32_t pct;

    if (offset >= -deadzone && offset <= deadzone) return 0;

    if (offset > 0) {
        pct = offset * 100 / (RC_PULSE_MAX - center);
    } else {
        pct = offset * 100 / (center - RC_PULSE_MIN);
    }

    pct = constrain(pct, -100, 100);
    return reversed ? -pct : pct;
}

static void motor_write_signed(MotorChannel ch, int32_t percent)
{
    percent = constrain(percent, -100, 100);

    if (percent > 0) {
        Motor_Forward(ch, (uint8_t)percent);
    } else if (percent < 0) {
        Motor_Reverse(ch, (uint8_t)(-percent));
    } else {
        Motor_Coast(ch);
    }
}

static void drive_write(int32_t left_pct, int32_t right_pct)
{
#if LEFT_MOTOR_REVERSED
    left_pct = -left_pct;
#endif
#if RIGHT_MOTOR_REVERSED
    right_pct = -right_pct;
#endif

    motor_write_signed(LEFT_MOTOR_CHANNEL, left_pct);
    motor_write_signed(RIGHT_MOTOR_CHANNEL, right_pct);
}

/* ---- Differential Tank Mixing ---- */
static void rc_to_motor(int32_t *left_pct, int32_t *right_pct,
                        int32_t ch1_filt, int32_t ch2_filt)
{
    int32_t throttle_pct = rc_axis_to_percent(ch2_filt, RC_CH2_CENTER,
                                              RC_CH2_DEADZONE,
                                              RC_THROTTLE_REVERSED);
    int32_t steering_pct = rc_axis_to_percent(ch1_filt, RC_CH1_CENTER,
                                              RC_CH1_DEADZONE,
                                              RC_STEERING_REVERSED);

#if !PIVOT_STEERING_ENABLE
    steering_pct = steering_pct * abs_i32(throttle_pct) / 100;
#endif

    int32_t left = throttle_pct + steering_pct;
    int32_t right = throttle_pct - steering_pct;
    int32_t max_pct = abs_i32(left);
    if (abs_i32(right) > max_pct) max_pct = abs_i32(right);

    if (max_pct > 100) {
        left = left * 100 / max_pct;
        right = right * 100 / max_pct;
    }

    *left_pct = left;
    *right_pct = right;
}

/* ---- Heartbeat ---- */
static void Heartbeat_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = HEARTBEAT_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HEARTBEAT_PORT, &gpio);
}

/* ---- Main ---- */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    Heartbeat_Init();
    Debug_UART_Init();
    Motor_Init();
    RC_Init();

    debug_print("F103 Dual Motor RC Car Ready\r\n");

    uint32_t last_control = 0;
    uint32_t last_debug = 0;
    uint32_t ch1_lost_since = 0;
    uint32_t ch2_lost_since = 0;
    int32_t last_left = 0, last_right = 0;
    uint8_t rc_was_valid = 0;
    uint8_t ch1_seen = 0, ch2_seen = 0;
    int32_t ch1_filt = RC_CH1_CENTER;
    int32_t ch2_filt = RC_CH2_CENTER;

    while (1) {
        uint32_t now = sys_tick_ms;
        uint32_t ch1_raw = RC_CH1_CENTER;
        uint32_t ch2_raw = RC_CH2_CENTER;
        uint8_t ch1_new = 0;
        uint8_t ch2_new = 0;

        /* ---- RC input: consume valid flags, update loss timer, EMA filter ---- */
        __disable_irq();
        if (rc_data.ch1_valid) {
            ch1_raw = rc_data.ch1;
            rc_data.ch1_valid = 0;
            ch1_new = 1;
        }
        if (rc_data.ch2_valid) {
            ch2_raw = rc_data.ch2;
            rc_data.ch2_valid = 0;
            ch2_new = 1;
        }
        __enable_irq();

        if (ch1_new) {
            ch1_lost_since = now;
            ch1_seen = 1;
            ch1_filt = (int32_t)ch1_raw;
        }
        if (ch2_new) {
            ch2_lost_since = now;
            ch2_seen = 1;
            ch2_filt = (int32_t)ch2_raw;
        }

        uint8_t rc_valid = ch1_seen && ch2_seen
                         && ((now - ch1_lost_since) <= SIGNAL_LOSS_MS)
                         && ((now - ch2_lost_since) <= SIGNAL_LOSS_MS);

        /* ---- Emergency stop: either channel lost for >100ms ---- */
        if (!rc_valid) {
            if (rc_was_valid) {
                Motor_EmergencyStop();
                rc_was_valid = 0;
                ch1_filt = RC_CH1_CENTER;
                ch2_filt = RC_CH2_CENTER;
                last_left = 0;
                last_right = 0;
                debug_print("Signal lost - emergency stop\r\n");
            }
        } else {
            rc_was_valid = 1;

            /* ---- Fixed-period control loop ---- */
            if (now - last_control >= CONTROL_PERIOD_MS) {
                last_control = now;

                rc_to_motor(&last_left, &last_right, ch1_filt, ch2_filt);
                drive_write(last_left, last_right);
            }
        }

        /* ---- Debug output at ~1Hz ---- */
        if (now - last_debug >= 1000) {
            last_debug = now;
            char buf[128];
            snprintf(buf, sizeof(buf),
                     "CH1:%ld CH2:%ld L:%ld%% R:%ld%% %s\r\n",
                     (long)ch1_filt, (long)ch2_filt,
                     (long)last_left, (long)last_right,
                     rc_was_valid ? "OK" : "LOST");
            debug_print(buf);
        }
    }
}

/* ---- SysTick ---- */
void SysTick_Handler(void)
{
    HAL_IncTick();
    sys_tick_ms++;

    /* Heartbeat: toggle at ~2Hz (every 250ms) */
    if ((sys_tick_ms % 250) == 0) {
        HAL_GPIO_TogglePin(HEARTBEAT_PORT, HEARTBEAT_PIN);
    }
}
