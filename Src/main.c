#include "main.h"

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

void print_field(const int *coord) {
        char sym = 0;
        for (int i = 0; i < 16; ++i) {
                for (int j = 0; j < 16; ++j) {
                        /* i is Y, j is X */
                        if (!((j + i) % 2)) {
                                sym = '@';
                        } else {
                                sym = '#';
                        }
                        if (i == coord[1] && j == coord[0]) {
                                /* coordinates of Knight */
                                sym = 'K';
                        }
                        if (i == coord[3] && j == coord[2]) {
                                sym = 'Q';
                        }
                        printf("%c ", sym);
                }
                printf("\r\n");
        }
}

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

bool queen_takes_over(const int *coord) {
        /* the same vert or hor */
        if (coord[0] == coord[2] ||
                        coord[1] == coord[3])
                return true;
        /* the same diagonal */
        if (max(coord[0], coord[2]) - min(coord[0], coord[2]) ==
                max(coord[1], coord[3]) - min(coord[1], coord[3]))
                return true;
        return false;
}
bool knight_takes_over(const int *coord) {
        if (max(coord[0], coord[2]) - min(coord[0], coord[2]) == 1 &&
                        max(coord[1], coord[3]) - min(coord[1], coord[3]) == 2)
                return true;
        if (max(coord[0], coord[2]) - min(coord[0], coord[2]) == 2 &&
                        max(coord[1], coord[3]) - min(coord[1], coord[3]) == 1)
                return true;
        return false;
}
void Leonardo_Task(void *pvParameters)
{
        char *prompt[] = {
                "Enter the x of the knight: ",
                "Enter the y of the knight: ",
                "Enter the x of the queen: ",
                "Enter the y of the queen: "
        };
        int coord[4] = {0, 0, 0, 0};
        vTaskDelay(pdMS_TO_TICKS(100));
	printf("leo task is here!\r\n");
	while (1) {
                for (int i = 0; i < 4; ++i) {
                        printf("%s\r\n", prompt[i]);
                        while(!scanf("%d", &coord[i])
                                || coord[i] < 0 || coord[i] > 15) {
                                printf("Enter valid number from 0 to 15!\r\n");
                                /* clear stdin */
                                fflush(stdin);
                        }
                        printf("entered number: %d\r\n", coord[i]);
                }
                if (queen_takes_over(coord)) {
                        printf("Queen takes over Knight\r\n");
                }
                if (knight_takes_over(coord)){
                        printf("Knight takes over Queen\r\n");
                }
                print_field(coord);
                printf("Success!\r\n\r\n");
        }
}

void BlinkLed_Task(void * pvParameters)
{
	uint16_t cnt = 0;
	uint16_t i = 0;
	bool button_was_pressed = false;
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
		if(LL_GPIO_IsInputPinSet(GPIOE, LL_GPIO_PIN_10)) {
			button_was_pressed = false;
		}
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

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
        tty_driver_init();
	xTaskCreate(BlinkLed_Task, "LED", 256, NULL,
                        BLINK_TASK_PRIORITY, NULL);
	xTaskCreate(Leonardo_Task, "Leo", 256, NULL,
			LEO_TASK_PRIORITY, NULL);
	vTaskStartScheduler();
	/*  should never get here */
	GPIOE->BSRR = (LL_GPIO_PIN_15 << 16);
	while (1) {  }
}
