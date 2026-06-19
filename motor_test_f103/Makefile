TARGET   := motor_test_f103
MCU      := STM32F103xB

CC       := arm-none-eabi-gcc
OBJCOPY  := arm-none-eabi-objcopy
SIZE     := arm-none-eabi-size

HAL_DIR  := ./STM32CubeF1

ifeq ($(wildcard $(HAL_DIR)/.),)
$(error "STM32CubeF1 未找到!")
endif

CPU      := -mcpu=cortex-m3
FPU      := -mfloat-abi=soft
CFLAGS   := $(CPU) $(FPU) -mthumb -Wall -Wextra -O1 -g3 \
            -ffunction-sections -fdata-sections -D$(MCU) -DUSE_HAL_DRIVER

LDFLAGS  := $(CPU) $(FPU) -mthumb -specs=nano.specs -specs=nosys.specs \
            -Wl,--gc-sections -Wl,-Map=$(TARGET).map \
            -T startup/STM32F103C8Tx_FLASH.ld

INC      := -I Inc \
            -I $(HAL_DIR)/Drivers/CMSIS/Device/ST/STM32F1xx/Include \
            -I $(HAL_DIR)/Drivers/CMSIS/Include \
            -I $(HAL_DIR)/Drivers/STM32F1xx_HAL_Driver/Inc

HAL_SRC  := $(HAL_DIR)/Drivers/STM32F1xx_HAL_Driver/Src
HAL_SRCS := $(HAL_SRC)/stm32f1xx_hal.c \
            $(HAL_SRC)/stm32f1xx_hal_cortex.c \
            $(HAL_SRC)/stm32f1xx_hal_gpio.c \
            $(HAL_SRC)/stm32f1xx_hal_rcc.c \
            $(HAL_SRC)/stm32f1xx_hal_rcc_ex.c \
            $(HAL_SRC)/stm32f1xx_hal_tim.c \
            $(HAL_SRC)/stm32f1xx_hal_tim_ex.c \
            $(HAL_SRC)/stm32f1xx_hal_flash.c \
            $(HAL_SRC)/stm32f1xx_hal_uart.c
USER_SRC := Src/main.c Src/motor.c Src/rc.c Src/system_stm32f1xx.c
STARTUP  := startup/startup_stm32f103xb.s
SRCS     := $(STARTUP) $(USER_SRC) $(HAL_SRCS)

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

flash: $(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
	    -c "program $(TARGET).elf verify reset exit"

dap: $(TARGET).elf
	sudo openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg \
	    -c "program $(TARGET).elf verify reset exit"

isp: $(TARGET).hex
	@echo "请先将BOOT0接3.3V, 按RESET进入ISP模式"
	sudo stm32flash -w $(TARGET).hex -v -g 0x0 /dev/ttyUSB0

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).map

.PHONY: all flash dap isp clean
