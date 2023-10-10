#include "main.h"

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

static void GPIO_Init(void)
{
	LL_GPIO_InitTypeDef leds = {
		.Pin = LL_GPIO_PIN_13 | LL_GPIO_PIN_14 | LL_GPIO_PIN_15,
		.Mode = LL_GPIO_MODE_OUTPUT,
		.Speed = LL_GPIO_SPEED_FREQ_LOW,
		.OutputType = LL_GPIO_OUTPUT_PUSHPULL,
		.Pull = LL_GPIO_PULL_NO,
	};
	LL_GPIO_InitTypeDef buttons = {
		.Pin = LL_GPIO_PIN_10,
		.Mode = LL_GPIO_MODE_INPUT,
		.Pull = LL_GPIO_PULL_UP,
	};
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOE);
	LL_GPIO_Init(GPIOE, &leds);
	LL_GPIO_Init(GPIOE, &buttons);
	// for leds to went out
	LL_GPIO_WriteOutputPort(GPIOE, 0xffffU);
}

void BlinkLed_Task(void * pvParameters)
{

	// printf("MISHA MOLODETS!\r\n");
	while (1) {
		usart1_tx("JOPA\r\n", 4);
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
extern void syncronisation_init(void);
int main(void)
{
	/*  TODO Dereference null pointer here TODO */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
	/*  figure out with 0 group */
	NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
	NVIC_SetPriority(SysTick_IRQn,
		NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));

	NVIC_SetPriority(DMA2_Stream7_IRQn,
		NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 10, 0));
	SystemClock_Config();
	GPIO_Init();
	syncronisation_init(); /* method for creation mutexes, semaphores... */
	usart1_init();
	/* usart1_tx("JOPA\r\n", 6);
	usart1_tx("JOPA\r\n", 6); */
	xTaskCreate(BlinkLed_Task, "LED", 128, NULL, 2, NULL);
	/* blinky_tm = xTimerCreate("LED, mount", pdMS_TO_TICKS(BLINK_NOT_MOUNTED),
				pdTRUE, NULL, Mount_Status_Cb); */
	/* xTaskCreate(Usb_Device_Task, "USBD", 4096, NULL, configMAX_PRIORITIES-1, NULL);
	xTaskCreate(Usb_CDC_Task, "USB_CDC", 128, NULL, configMAX_PRIORITIES-2, NULL); */
	//xTimerStart(blinky_tm, 0);
	vTaskStartScheduler();
	/*  should never get here */
	GPIOE->BSRR = (LL_GPIO_PIN_15 << 16);
	while (1) {  }
}
