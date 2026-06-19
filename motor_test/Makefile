TARGET   := motor_test
MCU      := STM32F407xx

CC       := arm-none-eabi-gcc
OBJCOPY  := arm-none-eabi-objcopy
SIZE     := arm-none-eabi-size

HAL_DIR  := ./STM32CubeF4

ifeq ($(wildcard $(HAL_DIR)/.),)
$(error "STM32CubeF4 未找到! 请先运行: git clone --depth 1 https://github.com/STMicroelectronics/STM32CubeF4 $(HAL_DIR) && cd $(HAL_DIR) && git submodule update --init --depth 1 Drivers/STM32F4xx_HAL_Driver Drivers/CMSIS/Device/ST/STM32F4xx")
endif

CPU      := -mcpu=cortex-m4
FPU      := -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS   := $(CPU) $(FPU) -mthumb -Wall -Wextra -O2 -g3 \
            -ffunction-sections -fdata-sections -D$(MCU) -DUSE_HAL_DRIVER

LDFLAGS  := $(CPU) $(FPU) -mthumb -specs=nano.specs -specs=nosys.specs \
            -Wl,--gc-sections -Wl,-Map=$(TARGET).map \
            -T startup/STM32F407ZGTx_FLASH.ld

HAL_SRC  := $(HAL_DIR)/Drivers/STM32F4xx_HAL_Driver/Src
HAL_SRCS := $(HAL_SRC)/stm32f4xx_hal.c $(HAL_SRC)/stm32f4xx_hal_cortex.c \
            $(HAL_SRC)/stm32f4xx_hal_gpio.c $(HAL_SRC)/stm32f4xx_hal_rcc.c \
            $(HAL_SRC)/stm32f4xx_hal_rcc_ex.c $(HAL_SRC)/stm32f4xx_hal_tim.c \
            $(HAL_SRC)/stm32f4xx_hal_tim_ex.c $(HAL_SRC)/stm32f4xx_hal_pwr.c \
            $(HAL_SRC)/stm32f4xx_hal_pwr_ex.c $(HAL_SRC)/stm32f4xx_hal_flash.c \
            $(HAL_SRC)/stm32f4xx_hal_flash_ex.c $(HAL_SRC)/stm32f4xx_hal_flash_ramfunc.c \
            $(HAL_SRC)/stm32f4xx_hal_adc.c

USER_SRC := Src/main.c Src/motor.c Src/rc.c Src/system_stm32f4xx.c
STARTUP  := startup/startup_stm32f407xx.s
SRCS     := $(STARTUP) $(USER_SRC) $(HAL_SRCS)

INC      := -I Inc \
            -I $(HAL_DIR)/Drivers/CMSIS/Device/ST/STM32F4xx/Include \
            -I $(HAL_DIR)/Drivers/CMSIS/Include \
            -I $(HAL_DIR)/Drivers/STM32F4xx_HAL_Driver/Inc

OBJS := $(SRCS:.c=.o)
OBJS := $(OBJS:.s=.o)

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex
	@$(SIZE) $(TARGET).elf

$(TARGET).elf: $(OBJS)
	@echo "  LD    $@"
	@$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) $(INC) -c -o $@ $<

%.o: %.s
	@echo "  AS    $<"
	@$(CC) $(CPU) $(FPU) -mthumb -c -o $@ $<

$(TARGET).bin: $(TARGET).elf
	@$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	@$(OBJCOPY) -O ihex $< $@

# ---- 烧录 ----
# ST-Link
flash: $(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
	    -c "program $(TARGET).elf verify reset exit"

# CMSIS-DAP
dap: $(TARGET).elf
	sudo openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg \
	    -c "program $(TARGET).elf verify reset exit"

# USB DFU (BOOT0接3.3V → 按RESET)
dfu: $(TARGET).bin
	@echo "请先让 STM32F4 进入 DFU 模式: BOOT0 接 3.3V → 按 RESET"
	@read -p "准备好后按回车..." _
	sudo dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D $(TARGET).bin

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).map

.PHONY: all flash dap dfu clean
