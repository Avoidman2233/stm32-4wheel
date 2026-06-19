#ifndef __STM32F4XX_HAL_CONF_H
#define __STM32F4XX_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED

#define HSE_VALUE              8000000U
#define HSI_VALUE             16000000U
#define LSI_VALUE             32000U
#define LSE_VALUE             32768U
#define EXTERNAL_CLOCK_VALUE  12288000U
#define HSE_STARTUP_TIMEOUT   100U
#define LSE_STARTUP_TIMEOUT   5000U
#define VDD_VALUE             3300U
#define TICK_INT_PRIORITY     0x0FU
#define USE_RTOS              0U
#define PREFETCH_ENABLE       1U

/* #define USE_FULL_ASSERT  1U */
#ifdef  USE_FULL_ASSERT
  #define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t* file, uint32_t line);
#else
  #define assert_param(expr) ((void)0U)
#endif

#include "stm32f4xx.h"

#ifdef HAL_MODULE_ENABLED
  #include "stm32f4xx_hal.h"
#endif
#ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32f4xx_hal_dma.h"
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32f4xx_hal_gpio.h"
#endif
#ifdef HAL_TIM_MODULE_ENABLED
  #include "stm32f4xx_hal_tim.h"
#endif
#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32f4xx_hal_rcc.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
  #include "stm32f4xx_hal_pwr.h"
#endif
#ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32f4xx_hal_cortex.h"
#endif
#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32f4xx_hal_flash.h"
#endif
#ifdef HAL_ADC_MODULE_ENABLED
  #include "stm32f4xx_hal_adc.h"
#endif
#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32f4xx_hal_rcc_ex.h"
#endif
#ifdef HAL_TIM_MODULE_ENABLED
  #include "stm32f4xx_hal_tim_ex.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
  #include "stm32f4xx_hal_pwr_ex.h"
#endif
#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32f4xx_hal_flash_ex.h"
  #include "stm32f4xx_hal_flash_ramfunc.h"
#endif

#ifdef __cplusplus
}
#endif
#endif
