#include "main.h"
#include <ctype.h>
/* simple tty driver over uart.
 * everything is simple for tx - just push n bytes out
 * but for rx we should buffer input until \n is encountered
 */

/* funtion for transfering num bytes over usart1 dma stream7 */

/* also we should remember that stack variables are located on ccmram
 * that cannot be accessed by dma, so we should pass only global arrays */

void usart1_init(void)
{
	LL_GPIO_InitTypeDef usart1_gpio = {
		.Pin = LL_GPIO_PIN_9 | LL_GPIO_PIN_10,
		.Mode = LL_GPIO_MODE_ALTERNATE,
		.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH,
		.OutputType = LL_GPIO_OUTPUT_PUSHPULL,
		.Pull = LL_GPIO_PULL_NO,
		.Alternate = LL_GPIO_AF_7
	};
	LL_USART_InitTypeDef usart1_config = {
		.BaudRate = 115200U,
		.DataWidth = LL_USART_DATAWIDTH_8B,
		.StopBits = LL_USART_STOPBITS_1,
		.Parity = LL_USART_PARITY_NONE,
		.TransferDirection = LL_USART_DIRECTION_TX_RX,
		.HardwareFlowControl = LL_USART_HWCONTROL_NONE,
		.OverSampling = LL_USART_OVERSAMPLING_8
	};
	__NVIC_EnableIRQ(USART1_IRQn);
	NVIC_SetPriority(USART1_IRQn,
		NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 10, 0));
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
	LL_GPIO_Init(GPIOA, &usart1_gpio);
	LL_USART_Init(USART1, &usart1_config);
	LL_USART_Enable(USART1);
        /* we dont need to clock gpioa anymore */
	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
}

const char *tx_buf_ptr;
uint32_t tx_buf_len;
extern EventGroupHandle_t ev1;
extern SemaphoreHandle_t usart1_tx_mutex;
void usart1_tx(const char *buf, const uint32_t num)
{
	if (num == 0) {
		return;
	}
	/* hold mutex */
	xSemaphoreTake(usart1_tx_mutex, portMAX_DELAY);
        tx_buf_ptr = buf;
        tx_buf_len = num;
        LL_USART_EnableIT_TXE(USART1);
	xEventGroupWaitBits(ev1, USART1_TX_SEM_BIT,
                                    pdTRUE, pdTRUE, portMAX_DELAY);
	xSemaphoreGive(usart1_tx_mutex);
}

extern QueueHandle_t usart1_rx_queue;
void USART1_IRQHandler(void)
{
	char byte;
        static uint32_t tx_ind = 0;
        BaseType_t xHigherPriorityTaskWoken;

        if (LL_USART_IsActiveFlag_TXE(USART1) &&
                                        LL_USART_IsEnabledIT_TXE(USART1)) {
                LL_USART_TransmitData8(USART1, tx_buf_ptr[tx_ind++]);
                if (tx_ind == tx_buf_len) {
                        tx_ind = 0;
                        LL_USART_DisableIT_TXE(USART1);
                        xEventGroupSetBitsFromISR(ev1, USART1_TX_SEM_BIT,
                                                &xHigherPriorityTaskWoken);
                        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
            return;
        }
        if (LL_USART_IsActiveFlag_RXNE(USART1) &&
                                        LL_USART_IsEnabledIT_RXNE(USART1)) {
		byte = LL_USART_ReceiveData8(USART1);
		xQueueSendFromISR(usart1_rx_queue, &byte, pdFALSE);
                return;
	}
}

extern SemaphoreHandle_t tty_mutex;
extern TaskHandle_t leo_tsk_hdl;
extern char user_input[64];

/* maybe someday i'll make support for escape sequences */
void TTY_Task(void *pvParameters)
{
	uint8_t cnt = 0;
        char byte;
	LL_USART_EnableIT_RXNE(USART1);
	usart1_tx("tty task is here!\r\n", 19);
	while(1) {
		xSemaphoreTake(tty_mutex, portMAX_DELAY);
		/* priority of leonardo task should be higher */
		xTaskNotifyGive(leo_tsk_hdl);
		while (1) {
                        if (cnt == 64 ) {
                                user_input[0] = '\0';
                                cnt = 0;
                                printf("Stupid!\r\n");
                                LL_USART_DisableIT_RXNE(USART1);
                        }
			xQueueReceive(usart1_rx_queue, &byte, portMAX_DELAY);
                        /* cool for debug */
                        // printf("\r\nnum: %x\r\n", byte);
                        usart1_tx(&byte, 1); // echo char back
                        if ('\r' == byte) {
                                printf("\r\n");
				user_input[cnt] = '\0';
				cnt = 0;
				xSemaphoreGive(tty_mutex);
                                break;
			}
                        else if ('\b' == byte) { /* backspace */
				if (cnt > 0) {
					user_input[--cnt] = '\0';
                                        printf("\r%s \b", user_input);
                                        fflush(stdout);
				}
			}
                        else if (byte >= 0x20 && byte <= 0x7E) { /* is printable */
                                user_input[cnt++] = byte;
                        }
		}
	}
}
