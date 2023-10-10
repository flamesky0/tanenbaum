cmake_minimum_required(VERSION 3.20)
project(tanenbaum)
enable_language(C ASM)

#FreeRTOS
set(FREERTOS_CONFIG_FILE_DIRECTORY "${CMAKE_SOURCE_DIR}/Inc" CACHE STRING "")
set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "")
#set(FREERTOS_HEAP ${CMAKE_SOURCE_DIR}/Src/newlib_syscalls.c)
add_subdirectory(FreeRTOS)

#LwIP
set(LWIP_DIR "${CMAKE_SOURCE_DIR}/LwIP" CACHE STRING "")
set(LWIP_INCLUDE_DIRS PUBLIC
	${LWIP_DIR}/include
	${LWIP_DIR}/include/LwIP
	${LWIP_DIR}/ports/freertos/include
	${LWIP_DIR}/ports/freertos/include/arch
	${CMAKE_SOURCE_DIR}/Inc
	${CMAKE_SOURCE_DIR}/FreeRTOS/include
	CACHE STRING "")
include(${CMAKE_SOURCE_DIR}/LwIP/Filelists.cmake)

#tinyusb
add_subdirectory(tinyusb)

#Drivers prefix
set(DSRC Drivers/Src)

#Sources to compile
set(SRC
	startup_stm32f407xx.s
	Src/main.c
	Src/stm32f4xx_it.c
	Src/system_stm32f4xx.c
	# Src/newlib_syscalls.c
	Src/usb_descriptors.c
	Src/usart1_driver.c
	Src/newlib_syscalls.c
	Src/sync.c
	${DSRC}/stm32f4xx_ll_utils.c
	${DSRC}/stm32f4xx_ll_dma.c
	${DSRC}/stm32f4xx_ll_usart.c
	${DSRC}/stm32f4xx_ll_rcc.c
	${DSRC}/stm32f4xx_ll_gpio.c
	)
set(EXEC ${PROJECT_NAME}.out)
add_executable(${EXEC} ${SRC})
target_link_libraries(${EXEC} PRIVATE lwipcore)
target_link_libraries(${EXEC} PUBLIC freertos_kernel freertos_kernel_port)
target_link_libraries(${EXEC} PRIVATE tinyusb)
add_definitions(-DUSE_FULL_LL_DRIVER)

target_include_directories(${EXEC} PUBLIC
	Inc/
	Inc/arch
	Drivers/Inc
	)

#create hex file and firmware
add_custom_command(TARGET ${EXEC}
	POST_BUILD
	COMMAND arm-none-eabi-size ${EXEC}
	COMMAND arm-none-eabi-objcopy -O ihex ${EXEC} ${PROJECT_NAME}.hex
	COMMAND arm-none-eabi-objcopy -O binary ${EXEC} ${PROJECT_NAME}.bin
)
