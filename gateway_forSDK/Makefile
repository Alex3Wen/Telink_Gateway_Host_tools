################################################################################
# Automatically-generated file. Do not edit!
################################################################################
#export PATH=$PATH:/opt/gcc-linaro-arm-linux-gnueabihf-4.8-2013.06_linux/bin
RM := rm -rf
GCC := arm-arago-linux-gnueabi-gcc
#GCC := gcc-4
# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
./appCmd.c \
./socCmd.c \
./nodes.c \
./server.c \
./main.c

OBJS += \
./appCmd.o \
./socCmd.o \
./nodes.o \
./server.o \
./main.o



# Each subdirectory must supply rules for building sources it contributes
%.o: %.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	$(GCC) -I./ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif


# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: gateway

# Tool invocations
gateway: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross GCC Linker'
	$(GCC)  -o "gateway" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) gateway
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
