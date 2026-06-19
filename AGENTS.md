# AGENTS.md

四轮移动平台固件仓库，两个 MCU 目标共享相同的电机驱动 + RC 遥控架构。

## 两个目标，不要搞混

| 目录 | MCU | 内核 | FPU | 用途 |
|------|-----|------|-----|------|
| `motor_test/` | STM32F407ZGTx | Cortex-M4 | hard (fpv4-sp-d16) | 原始正反转测试示例 |
| `motor_test_f103/` | STM32F103C8T6 | Cortex-M3 | soft | 完整版：差速混合 + RC 控制 + 心跳灯 + ADS1115 |

**两个目录之间有编译产物 `.o` 文件残留**——切换目标时先 `make clean` 再 `make`，否则链接阶段可能捡到错误架构的目标文件。

`CLAUDE.md` 的内容以 F407 为主，F103 细节在此补充。

## 构建前置条件

每个目标目录都需要各自克隆 HAL 库：

```bash
# F407
cd motor_test
git clone --depth 1 https://github.com/STMicroelectronics/STM32CubeF4 STM32CubeF4
cd STM32CubeF4 && git submodule update --init --depth 1 Drivers/STM32F4xx_HAL_Driver Drivers/CMSIS/Device/ST/STM32F4xx && cd ..

# F103
cd motor_test_f103
git clone --depth 1 https://github.com/STMicroelectronics/STM32CubeF1 STM32CubeF1
cd STM32CubeF1 && git submodule update --init --depth 1 Drivers/STM32F1xx_HAL_Driver Drivers/CMSIS/Device/ST/STM32F1xx && cd ..
```

Makefile 会检测 HAL 目录是否存在，缺少时给出明确错误。

## 构建 & 烧录命令

```bash
make              # 编译生成 .elf / .bin / .hex
make flash        # ST-Link (OpenOCD)
make dap          # CMSIS-DAP (F103 需 sudo)
make dfu          # F407 USB DFU (BOOT0 接 3.3V → 按 RESET)
make isp          # F103 串口 ISP (BOOT0=1, BOOT1=0, 用 stm32flash)
```

## 平台差异要点

### F407 → F103 迁移时注意

- **TIM5 不存在于 F103**：F103 用 TIM2_CH2 替代。RC 模块中定时器外设名不同。
- **时钟树完全不同**：F407 = 168MHz (HSI→PLL)；F103 = 72MHz (HSE 8MHz×PLL9)。APB1 定时器时钟在 F103 上是 ×2（72MHz），而非 F407 的 /4。
- **TIM_ICPolarity 命名**：F1 HAL 是 `TIM_ICPOLARITY_xxx`，F4 HAL 是 `TIM_ICPOLARITY_xxx`。如果从 STM32CubeMX 重新生成代码，注意 HAL 版本差异。
- **ADC 包含 HAL 文件不同**：F407 Makefile 包含了 `stm32f4xx_hal_adc.c`（虽然当前未使用），F103 不需要。

### F103 额外模块（F407 没有）

- `heartbeat.c`：PA4 外接 LED 心跳闪烁，在 SysTick_Handler 中轮询
- `ads1115.c`：I2C2 (PB10/PB11) 外接 ADS1115 16 位 ADC，用于电池电压/电流采样
- `debug_uart.c`：UART 调试输出（仅初始化函数）

## 电机驱动关键约束

1. **引脚悬空 = 高电平**：驱动板输入脚悬空默认 H。上电初始化必须先将 INA/INB 全部拉高进入滑行态，再配置 PWM。
2. **方向切换必须先制动 ≥100ms**：`Motor_Forward/Reverse` 内部自动插入 `MOTOR_DIR_DEADTIME_MS` 制动死区。修改这个宏时要确保 ≥100ms，否则可能击穿驱动芯片。
3. **驱动逻辑表参见** `电机驱动板参数.md`。

## RC 遥控关键约束

- 接收机（R8EF）输出约 1V 弱信号，**需经电平转换（三极管/MOSFET）提升到 3.3V** 才能被 GPIO 识别。
- 若电平转换电路是反相的，需将 `rc.c` 中的捕获极性改为 `TIM_ICPOLARITY_FALLING`。
- 脉宽有效性校验：800-2200μs 范围过滤（定义在 `rc.h`）。
- F103 版本 `main.c` 实现了信号丢失保护：任一通道连续 100ms 无效 → 急停。

## 引脚分配参考

F407 引脚参见 `CLAUDE.md`。F103 引脚定义在 `motor_test_f103/Inc/motor.h` 和 `rc.h` 中。

物理引脚排布（焊接参考）：`stm32f4xx引脚.md`。

## 编码约定

- C99，`__` 双下划线头文件保护
- 模块 API 用 `Module_Function` 命名
- `uint8_t percent` 表示 0-100 的百分比占空比
- 电机方向控制使用 `MotorChannel` / `MotorState` 枚举，不使用魔术数字
