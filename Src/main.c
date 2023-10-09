#include "main.h"
/* FreeRTOS */
#include <FreeRTOS.h>
#include "list.h"
#include "queue.h"
#include "task.h"
#include "timers.h"
/* tinyusb */


TimerHandle_t blinky_tm;

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum {
	BLINK_NOT_MOUNTED = 250,
	BLINK_MOUNTED = 1000,
	BLINK_SUSPENDED = 2500
};

void SystemClock_Config(void)
{
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);
	while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_5) {

	}
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
	LL_RCC_HSE_Enable();

/* Wait till HSE is ready */
	while(LL_RCC_HSE_IsReady() != 1) {

	}
	LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 336, LL_RCC_PLLP_DIV_2);
	LL_RCC_PLL_Enable();

/* Wait till PLL is ready */
	while(LL_RCC_PLL_IsReady() != 1) {

	}
	while (LL_PWR_IsActiveFlag_VOS() == 0)	{  }
	LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
	LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
	LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

/* Wait till System clock is ready */
	while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {  }
	LL_Init1msTick(168000000);
	LL_SetSystemCoreClock(168000000);
}

/* static void USART1_Init(void)
{
	// file stm32f4xx.h ldr exclusive instructions figure out
	LL_USART_InitTypeDef usart1;
	usart1.BaudRate 	   = 115200U;
	usart1.DataWidth 	   = LL_USART_DATAWIDTH_8B;
	usart1.StopBits            = LL_USART_STOPBITS_1;
	usart1.Parity              = LL_USART_PARITY_NONE ;
	usart1.TransferDirection   = LL_USART_DIRECTION_TX_RX;
	usart1.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
	usart1.OverSampling        = LL_USART_OVERSAMPLING_16;
	LL_USART_Init(USART1, &usart1);
} */

static void GPIO_Init(void)
{
	LL_GPIO_InitTypeDef leds = {
		.Pin = LL_GPIO_PIN_13 | LL_GPIO_PIN_14 | LL_GPIO_PIN_15,
		.Mode = LL_GPIO_MODE_OUTPUT,
		.Speed = LL_GPIO_SPEED_FREQ_LOW,
		.OutputType = LL_GPIO_OUTPUT_PUSHPULL,
		.Pull = LL_GPIO_PULL_NO,
	};
	LL_GPIO_InitTypeDef switches = {
		.Pin = LL_GPIO_PIN_10,
		.Mode = LL_GPIO_MODE_INPUT,
		.Pull = LL_GPIO_PULL_UP,
	};
	/* LL_GPIO_InitTypeDef usart1 = {
		.Pin = LL_GPIO_PIN_9 | LL_GPIO_PIN_10,
		.Mode = LL_GPIO_MODE_ALTERNATE,
		.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH,
		.OutputType = LL_GPIO_OUTPUT_PUSHPULL,
		.Pull = LL_GPIO_PULL_NO,
		.Alternate = LL_GPIO_AF_7;
	}; */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOE);
	/*  TODO try disable GPIOA after initialisation */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	LL_GPIO_Init(GPIOE, &leds);
	LL_GPIO_Init(GPIOE, &switches);
	// LL_GPIO_Init(GPIOA, &usart1);
	/* make leds go out */
	LL_GPIO_WriteOutputPort(GPIOE, 0xffffU);
}

void Error_Handler(void)
{
	__disable_irq();
	/*  pull pin down, make led light */
	GPIOE->BSRR = (LL_GPIO_PIN_15 << 16);
	while (1) {  }
}

void BlinkLed_Task(void * pvParameters)
{
	while (1) {
		LL_GPIO_TogglePin(GPIOE, LL_GPIO_PIN_14);
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

/* Callback that has variable shoot time
 * depending on usb device mount status */
void Mount_Status_Cb(TimerHandle_t xTimer)
{
	(void) xTimer;
	LL_GPIO_TogglePin(GPIOE, LL_GPIO_PIN_13);
}

// malloc
// printf
int main(void)
{
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
	/*  figure out with 0 group */
	NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
	NVIC_SetPriority(SysTick_IRQn,
			NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
	SystemClock_Config();
	GPIO_Init();
	xTaskCreate(BlinkLed_Task, "LED", 128, NULL, 2, NULL);
	printf("%d", 4);
	blinky_tm = xTimerCreate("LED, mount", pdMS_TO_TICKS(BLINK_NOT_MOUNTED),
				pdTRUE, NULL, Mount_Status_Cb);
	/* xTaskCreate(Usb_Device_Task, "USBD", 4096, NULL, configMAX_PRIORITIES-1, NULL);
	xTaskCreate(Usb_CDC_Task, "USB_CDC", 128, NULL, configMAX_PRIORITIES-2, NULL); */
	xTimerStart(blinky_tm, 0);
	vTaskStartScheduler();
	/*  should never get here */
	GPIOE->BSRR = (LL_GPIO_PIN_15 << 16);
	while (1) {  }
}
