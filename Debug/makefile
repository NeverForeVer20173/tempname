################################################################################
# 自动生成的文件。不要编辑！
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include rt-thread/src/subdir.mk
-include rt-thread/libcpu/arm/cortex-m4/subdir.mk
-include rt-thread/libcpu/arm/common/subdir.mk
-include rt-thread/components/net/lwip/port/subdir.mk
-include rt-thread/components/net/lwip/lwip-2.0.3/src/netif/subdir.mk
-include rt-thread/components/net/lwip/lwip-2.0.3/src/core/ipv4/subdir.mk
-include rt-thread/components/net/lwip/lwip-2.0.3/src/core/subdir.mk
-include rt-thread/components/net/lwip/lwip-2.0.3/src/apps/ping/subdir.mk
-include rt-thread/components/net/lwip/lwip-2.0.3/src/api/subdir.mk
-include rt-thread/components/libc/compilers/newlib/subdir.mk
-include rt-thread/components/libc/compilers/common/subdir.mk
-include rt-thread/components/finsh/subdir.mk
-include rt-thread/components/drivers/wlan/subdir.mk
-include rt-thread/components/drivers/serial/subdir.mk
-include rt-thread/components/drivers/rtc/subdir.mk
-include rt-thread/components/drivers/misc/subdir.mk
-include rt-thread/components/drivers/ipc/subdir.mk
-include rt-thread/components/drivers/i2c/subdir.mk
-include rt-thread/components/drivers/hwtimer/subdir.mk
-include packages/wlan_cyw43012-v0.0.1/wifi-host-driver/WiFi_Host_Driver/src/bus_protocols/subdir.mk
-include packages/wlan_cyw43012-v0.0.1/wifi-host-driver/WiFi_Host_Driver/src/subdir.mk
-include packages/wlan_cyw43012-v0.0.1/wifi-host-driver/WiFi_Host_Driver/resources/resource_imp/subdir.mk
-include packages/wlan_cyw43012-v0.0.1/wifi-host-driver/WiFi_Host_Driver/resources/firmware/COMPONENT_43012/subdir.mk
-include packages/wlan_cyw43012-v0.0.1/wifi-host-driver/WiFi_Host_Driver/resources/clm/COMPONENT_43012/subdir.mk
-include packages/wlan_cyw43012-v0.0.1/whd-bsp-integration/COMPONENT_LWIP/subdir.mk
-include packages/wlan_cyw43012-v0.0.1/whd-bsp-integration/subdir.mk
-include packages/wlan_cyw43012-v0.0.1/abstraction-rtos/source/COMPONENT_FREERTOS/subdir.mk
-include packages/wlan_cyw43012-v0.0.1/abstraction-rtos/source/subdir.mk
-include packages/wlan_cyw43012-v0.0.1/subdir.mk
-include packages/u8g2-c-latest/src/subdir.mk
-include packages/u8g2-c-latest/port/subdir.mk
-include packages/cJSON-v1.7.17/subdir.mk
-include packages/FreeRTOS_Wrapper-v1.0.0/FreeRTOS/portable/rt-thread/subdir.mk
-include packages/FreeRTOS_Wrapper-v1.0.0/FreeRTOS/subdir.mk
-include libs/TARGET_RTT-062S2/config/GeneratedSource/subdir.mk
-include libs/TARGET_RTT-062S2/COMPONENT_CM4/TOOLCHAIN_GCC_ARM/subdir.mk
-include libs/TARGET_RTT-062S2/COMPONENT_CM4/subdir.mk
-include libs/TARGET_RTT-062S2/subdir.mk
-include libraries/IFX_PSOC6_HAL/retarget-io/subdir.mk
-include libraries/IFX_PSOC6_HAL/psoc6cm0p/COMPONENT_CM0P_SLEEP/subdir.mk
-include libraries/IFX_PSOC6_HAL/mtb-pdl-cat1/drivers/source/TOOLCHAIN_GCC_ARM/subdir.mk
-include libraries/IFX_PSOC6_HAL/mtb-pdl-cat1/drivers/source/subdir.mk
-include libraries/IFX_PSOC6_HAL/mtb-pdl-cat1/devices/COMPONENT_CAT1A/source/subdir.mk
-include libraries/IFX_PSOC6_HAL/mtb-hal-cat1/source/subdir.mk
-include libraries/IFX_PSOC6_HAL/mtb-hal-cat1/COMPONENT_CAT1A/source/triggers/subdir.mk
-include libraries/IFX_PSOC6_HAL/mtb-hal-cat1/COMPONENT_CAT1A/source/pin_packages/subdir.mk
-include libraries/IFX_PSOC6_HAL/capsense/subdir.mk
-include libraries/HAL_Drivers/subdir.mk
-include board/ports/subdir.mk
-include board/subdir.mk
-include applications/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
rtthread.hex \

SECONDARY_SIZE += \
rtthread.siz \


# 所有目标
all: rtthread.elf secondary-outputs

# 工具调用
rtthread.elf: $(OBJS) $(USER_OBJS)
	arm-none-eabi-gcc -T "D:\RT-ThreadStudio\workspace\RT-AIOT\board\linker_scripts\link.ld" -Xlinker --gc-sections -L"D:\RT-ThreadStudio\workspace\RT-AIOT\libraries\IFX_PSOC6_HAL\lib" -Wl,-Map,"rtthread.map" -Xlinker --cref -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -ffunction-sections -fdata-sections -Wl,--gc-sections,-Map=rtthread.map,-cref,-u,Reset_Handler -o "rtthread.elf" $(OBJS) $(USER_OBJS) $(LIBS)

rtthread.hex: rtthread.elf
	arm-none-eabi-objcopy -O ihex "rtthread.elf"  "rtthread.hex"

rtthread.siz: rtthread.elf
	arm-none-eabi-size --format=berkeley "rtthread.elf"

# 其他目标
clean:
	-$(RM) $(OBJS)$(SECONDARY_FLASH)$(SECONDARY_SIZE)$(ASM_DEPS)$(S_DEPS)$(S_UPPER_DEPS)$(C_DEPS) rtthread.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets
