# Tanenbaum
This is my personal project for learning operating systems, networks and computer design
I'll name it Tanenbaum

## Used technologies
I use **stm32f407vet6** mcu with FreeRTOS, LwIP, tinyusb, LL(Low Level) device drivers

I build it *without* using STM32CubeMX and HAL
(I can talk eight hours straight why I've come to such decision)

Also used provided by MCU's support package files such as linker script,
asm startup file, CMSIS regs definitions

I use CMake for building (gcc-arm-none-eabi toolchain)

## Current status
For now ready usart driver - tx - rx natively by irq,
newlib ported, so freertos uses its heap allocated on ccmram.
look at tag in git history

Commands to build project:

```
$ ./build
$ make -C Build/
```

