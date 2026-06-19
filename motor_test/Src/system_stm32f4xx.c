#include "stm32f4xx.h"

uint32_t SystemCoreClock = 16000000;
const uint8_t AHBPrescTable[16] = {0,0,0,0,0,0,0,0,1,2,3,4,6,7,8,9};
const uint8_t APBPrescTable[8]  = {0,0,0,0,1,2,3,4};

void SystemInit(void)
{
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));
#endif
    RCC->CR |= (uint32_t)0x00000001;
    RCC->CFGR = 0x00000000;
    RCC->CR &= (uint32_t)0xFEF6FFFF;
    RCC->PLLCFGR = 0x24003010;
    RCC->CR &= (uint32_t)0xFFFBFFFF;
    RCC->CIR = 0x00000000;
    SCB->VTOR = FLASH_BASE;
    SystemCoreClock = 16000000;
}

void SystemCoreClockUpdate(void)
{
    uint32_t tmp, pllvco, pllp, pllsource, pllm;
    tmp = RCC->CFGR & RCC_CFGR_SWS;
    switch (tmp) {
    case 0x00: SystemCoreClock = 16000000; break;
    case 0x04: SystemCoreClock = 8000000;  break;
    case 0x08:
        pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> 22;
        pllm = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;
        pllvco = ((pllsource != 0) ? 8000000 : 16000000) / pllm
               * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
        pllp = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> 16) + 1) * 2;
        SystemCoreClock = pllvco / pllp;
        break;
    default: SystemCoreClock = 16000000; break;
    }
    tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4)];
    SystemCoreClock >>= tmp;
}

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    (void)TickPriority;
    if (SysTick_Config(SystemCoreClock / 1000U) != 0) return HAL_ERROR;
    NVIC_SetPriority(SysTick_IRQn, 0x0F);
    return HAL_OK;
}

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}
