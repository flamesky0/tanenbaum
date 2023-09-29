#include "main.h"
#include <FreeRTOS.h>
#include "list.h"
#include "queue.h"
#include "task.h"


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
	LL_GPIO_InitTypeDef switches = {
		.Pin = LL_GPIO_PIN_10,
		.Mode = LL_GPIO_MODE_INPUT,
		.Pull = LL_GPIO_PULL_UP,
	};
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOE);
	LL_GPIO_Init(GPIOE, &leds);
	LL_GPIO_Init(GPIOE, &switches);
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

void BlinkLed_Task(void * pvParameters) {
	static int i = 0;
	while (1) {
		if (LL_GPIO_IsInputPinSet(GPIOE, LL_GPIO_PIN_10)) {
			/*  button is NOT pressed */
			GPIOE->BSRR = (LL_GPIO_PIN_13);
		} else {
			GPIOE->BSRR = (LL_GPIO_PIN_13 << 16);
		}
		if (10 == i) {
			LL_GPIO_TogglePin(GPIOE, LL_GPIO_PIN_14);
			i = 0;
		}
		++i;
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

int main(void)
{
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
	NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
	NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
	SystemClock_Config();
	GPIO_Init();
	xTaskCreate(BlinkLed_Task, "LED", 128, NULL, 5, NULL);
	vTaskStartScheduler();
	/*  should never get here */
	while (1) {  }
}
