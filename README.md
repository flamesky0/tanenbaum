![image](https://github.com/flamesky0/tanenbaum/assets/79990715/4e8aae02-015d-4b60-a1d8-61ace0419ad1)

# Tanenbaum
This is my personal project for learning operating systems, networks and computer design
I'll name it Tanenbaum

## Used technologies
I use **stm32f407vet6** mcu with FreeRTOS, LwIP, tinyusb, LL(Low Level) platform device drivers

I build it *without* using STM32CubeMX and HAL
(I can talk eight hours straight why I've come to such decision)

Used only provided by MCU's support package files such as linker script,
asm startup file, CMSIS regs definitions, almost header-only LL library.

I use CMake for building (gcc-arm-none-eabi toolchain)

## Current status
newlib ported, write, read, sbrk syscalls implemented. write and read work with uart, and sbrk allocates memory on ccmram
(core coupled memory with fixed-latency access time, 2 cycles). In order to achieve it linker script was amended. 

rtc (real-time clock) is working and clocked by LSE, powered by battery.

tinyusb is ported so enumeration succedes, mcu acts like hid device (mouse in my case), read data over adc from
joystick and sends movement information over usb to PC.

cli is working over uart so you can retrieve (or set) information about date and time from rtc, blink led.

Commands to build project:

```
$ ./build
$ make -C Build/
```
Commands to flash firmware:

```
$ ./flash
```
