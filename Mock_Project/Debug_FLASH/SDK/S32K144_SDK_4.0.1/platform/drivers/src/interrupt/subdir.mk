################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../SDK/S32K144_SDK_4.0.1/platform/drivers/src/interrupt/interrupt_manager.c 

OBJS += \
./SDK/S32K144_SDK_4.0.1/platform/drivers/src/interrupt/interrupt_manager.o 

C_DEPS += \
./SDK/S32K144_SDK_4.0.1/platform/drivers/src/interrupt/interrupt_manager.d 


# Each subdirectory must supply rules for building sources it contributes
SDK/S32K144_SDK_4.0.1/platform/drivers/src/interrupt/%.o: ../SDK/S32K144_SDK_4.0.1/platform/drivers/src/interrupt/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@SDK/S32K144_SDK_4.0.1/platform/drivers/src/interrupt/interrupt_manager.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


