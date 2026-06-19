# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

四轮移动平台固件，基于 STM32F407ZGTx（Cortex-M4），使用 STM32CubeF4 HAL 裸机开发，arm-none-eabi-gcc 工具链。双路电机 PWM 驱动 + RC 遥控接收。

## 构建与烧录

```bash
cd motor_test

# 首次使用需克隆 HAL 库（只需一次）
git clone --depth 1 https://github.com/STMicroelectronics/STM32CubeF4 STM32CubeF4
cd STM32CubeF4 && git submodule update --init --depth 1 Drivers/STM32F4xx_HAL_Driver Drivers/CMSIS/Device/ST/STM32F4xx && cd ..

# 编译
make           # 生成 motor_test.elf / .bin / .hex

# 烧录（三选一）
make flash     # ST-Link (OpenOCD)
make dap       # CMSIS-DAP
make dfu       # USB DFU 模式（先 BOOT0 接 3.3V → 按 RESET）
```

工具链依赖：`arm-none-eabi-gcc`、`openocd`（ST-Link / CMSIS-DAP 烧录时）、`dfu-util`（DFU 烧录时）。

## 代码架构

```
motor_test/
├── Src/
│   ├── main.c                  # 入口，循环正反转示例
│   ├── motor.c                 # 双路电机驱动（PWM + 方向控制）
│   ├── rc.c                    # RC 遥控 PWM 捕获（输入捕获中断）
│   └── system_stm32f4xx.c      # 系统初始化（FPU、时钟、SysTick）
├── Inc/
│   ├── motor.h                 # 电机模块 API 及引脚/PWM 宏定义
│   ├── rc.h                    # RC 模块 API 及通道定义
│   └── stm32f4xx_hal_conf.h    # HAL 模块裁剪配置
├── startup/
│   ├── startup_stm32f407xx.s   # 中断向量表 + Reset_Handler
│   └── STM32F407ZGTx_FLASH.ld  # 链接脚本（Flash 1024K, SRAM 128K, CCM 64K）
├── Makefile
└── STM32CubeF4/                # ST 官方 HAL 库（git clone 获取）
```

## 关键设计决策

### 电机驱动（motor.c）

- **驱动板逻辑**：采用 INA/INB + PWM 控制模式。INA=L/INB=L 为制动，INA=L/INB=H 为正转，INA=H/INB=L 为反转，INA=H/INB=H 为滑行（高阻态）。详见 `电机驱动板参数.md`。
- **方向切换保护**：正转↔反转切换时，`Motor_Forward()` / `Motor_Reverse()` 会自动插入 ≥100ms 的制动死区（`MOTOR_DIR_DEADTIME_MS`），防止反向冲击电流击穿驱动芯片。
- **引脚悬空**：驱动板输入引脚悬空时默认等效为高电平 H，初始化时将 INA/INB 全部拉高（滑行状态），确保上电安全。
- **PWM 频率**：统一 20kHz（`MOTOR_PWM_FREQ_HZ`），TIM1 挂在 APB2（168MHz），TIM3 挂在 APB1（84MHz），ARR 值由宏自动计算。
- **紧急停止**：`Motor_EmergencyStop()` 同时制动两个通道。

### RC 遥控（rc.c）

- 使用定时器输入捕获中断测量 PWM 脉宽（1000-2000μs，中点 1500μs）。
- CH1（转向）：PB6 → TIM4_CH1；CH2（油门）：PA1 → TIM5_CH2。
- 脉宽有效性校验：800-2200μs 范围过滤。

### 系统时钟

- HSI（16MHz）→ PLL：/16 × 336 / 2 = 168MHz SYSCLK。
- APB1 = 42MHz（/4），APB2 = 84MHz（/2）。

## 引脚分配（STM32F407）

| 功能 | 引脚 | 外设 |
|------|------|------|
| 电机1 PWM | PA8 | TIM1_CH1 |
| 电机2 PWM | PA6 | TIM3_CH1 |
| 电机1 INA | PB0 | GPIO |
| 电机1 INB | PB1 | GPIO |
| 电机2 INA | PB2 | GPIO |
| 电机2 INB | PB3 | GPIO |
| RC CH1（转向）| PB6 | TIM4_CH1 |
| RC CH2（油门）| PA1 | TIM5_CH2 |

完整 STM32F407 引脚物理排布参见 `stm32f4xx引脚.md`。
