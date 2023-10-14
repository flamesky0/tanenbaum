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
		.Pull = LL_GPIO_PULL_NO, /* it pulled up on the board */
	};
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOE);
	LL_GPIO_Init(GPIOE, &leds);
	LL_GPIO_Init(GPIOE, &buttons);
	// for leds to went out
	LL_GPIO_WriteOutputPort(GPIOE, 0xffffU);
}

/* Leonardo was my favorite Ninja Turtle
 * Now this is interpreter task that takes commads from user input and does sth
 */
char user_input[64];
extern SemaphoreHandle_t tty_mutex;
void Leonardo_Task(void *pvParameters)
{
	printf("leo task is here!\r\n");
	while (1) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		xSemaphoreTake(tty_mutex, portMAX_DELAY);
		/* doing some work with user_input[] ... */
		/* maybe just copy in local array */
		printf("I'm woken! (Leo)\r\n");
	        printf("I got \"%s\" string\r\n", user_input);
		xSemaphoreGive(tty_mutex);
	}
}

void BlinkLed_Task(void * pvParameters)
{
	uint16_t cnt = 0;
	uint16_t i = 0;
	bool button_was_pressed = false;
	usart1_tx("button task is here!\r\n", 22);
	while (1) {
		if (11 == ++i) { /* yeap, eleven */
			LL_GPIO_TogglePin(GPIOE, LL_GPIO_PIN_14);
			i = 0;
		}
		if (!LL_GPIO_IsInputPinSet(GPIOE, LL_GPIO_PIN_10) &&
				!button_was_pressed) {
                        printf("Button pressed in %uth time\r\n", ++cnt);
			button_was_pressed = true;
		}
		if(LL_GPIO_IsInputPinSet(GPIOE, LL_GPIO_PIN_10)) {
			button_was_pressed = false;
		}
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

/* Callback that has variable shoot time
 * depending on usb device mount status */
void Mount_Status_Cb(TimerHandle_t xTimer)
{
	(void) xTimer;
}

/* method for creation mutexes, semaphores, etc... */
extern void synchronisation_init(void);
TaskHandle_t leo_tsk_hdl;
extern void TTY_Task(void *pvParameters);
int main(void)
{
	/*  TODO Dereference null pointer here TODO */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
	/*  figure out with 0 group */
	NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
	NVIC_SetPriority(SysTick_IRQn,
		NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
	SystemClock_Config();
	GPIO_Init();
	synchronisation_init();
	usart1_init();
	xTaskCreate(BlinkLed_Task, "LED", 256, NULL, 2, NULL); /* priority 2 */
#define TTY_TASK_PRIORITY 30
	xTaskCreate(TTY_Task, "tty", 256, NULL, TTY_TASK_PRIORITY, NULL);
	xTaskCreate(Leonardo_Task, "Leo", 256, NULL,
			TTY_TASK_PRIORITY + 1, &leo_tsk_hdl);

	/* blinky_tm = xTimerCreate("LED, mount", pdMS_TO_TICKS(BLINK_NOT_MOUNTED),
				pdTRUE, NULL, Mount_Status_Cb); */
	/* xTaskCreate(Usb_Device_Task, "USBD", 4096, NULL, configMAX_PRIORITIES-1, NULL);
	xTaskCreate(Usb_CDC_Task, "USB_CDC", 128, NULL, configMAX_PRIORITIES-2, NULL); */
	// xTimerStart(blinky_tm, 0);
	vTaskStartScheduler();
	/*  should never get here */
	GPIOE->BSRR = (LL_GPIO_PIN_15 << 16);
	while (1) {  }
}
