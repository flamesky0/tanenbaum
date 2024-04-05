#include "main.h"

void SystemClock_Config(void)
{

	LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);
	LL_FLASH_EnableInstCache();
	LL_FLASH_EnableDataCache();
	LL_FLASH_EnablePrefetch();
	while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_5) {

	}
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
	LL_RCC_HSE_Enable();

	LL_PWR_EnableBkUpAccess();
	LL_RCC_LSE_Enable();
	while(LL_RCC_LSE_IsReady() != 1) {  }

	/* Wait till HSE is ready */
	while(LL_RCC_HSE_IsReady() != 1) {  }
	LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 336, LL_RCC_PLLP_DIV_2);
	/* Config clocks for usb controller */
	LL_RCC_PLL_ConfigDomain_48M(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 336, LL_RCC_PLLQ_DIV_7);
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

static void gpio_init(void)
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

void BlinkLed_Task(void * pvParameters)
{
	uint16_t cnt = 0;
	uint16_t i = 0;
	bool button_was_pressed = false;
	gpio_init();
        printf("blinkled task is here!\r\n");
	while (1) {
		if (10 == ++i) {
			LL_GPIO_TogglePin(GPIOE, LL_GPIO_PIN_14);
			i = 0;
		}
		if (!LL_GPIO_IsInputPinSet(GPIOE, LL_GPIO_PIN_10) &&
				!button_was_pressed) {
                        printf("Button pressed in %uth time\r\n", ++cnt);
			button_was_pressed = true;
		}
		if (LL_GPIO_IsInputPinSet(GPIOE, LL_GPIO_PIN_10)) {
			button_was_pressed = false;
		}
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

static void rtc_init() {
	LL_RTC_InitTypeDef rtc = {
		.HourFormat = LL_RTC_HOURFORMAT_24HOUR,
		.AsynchPrescaler = 127,
		.SynchPrescaler = 255
	};

	if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSE) {
		LL_RCC_ForceBackupDomainReset();
		LL_RCC_ReleaseBackupDomainReset();
		LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
	}
	LL_RCC_EnableRTC();
	LL_RTC_Init(RTC, &rtc);
	/* disable write protection back */
	LL_RTC_DisableWriteProtection(RTC);
}

int main(void)
{
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
	NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
	NVIC_SetPriority(SysTick_IRQn,
		NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));

	SystemClock_Config();
	rtc_init();
        tty_driver_init();
	usb_device_init();
	cli_setup();
	xTaskCreate(BlinkLed_Task, "LED", 256, NULL,
                        BLINK_TASK_PRIORITY, NULL);
	vTaskStartScheduler();
	/*  should never get here */
	GPIOE->BSRR = (LL_GPIO_PIN_15 << 16);
	while (1) {  }
}
