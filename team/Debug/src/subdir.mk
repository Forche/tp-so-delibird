################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/matcher.c \
../src/planner.c \
../src/resolver.c \
../src/team.c \
../src/trainer.c \
../src/util.c 

OBJS += \
./src/matcher.o \
./src/planner.o \
./src/resolver.o \
./src/team.o \
./src/trainer.o \
./src/util.o 

C_DEPS += \
./src/matcher.d \
./src/planner.d \
./src/resolver.d \
./src/team.d \
./src/trainer.d \
./src/util.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/Delibird/tp-2020-1c-Operavirus/operavirus-commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


