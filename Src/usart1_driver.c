#include "main.h"
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
		.Parity = LL_USART_PARITY_NONE, // TODO
		.TransferDirection = LL_USART_DIRECTION_TX_RX,
		.HardwareFlowControl = LL_USART_HWCONTROL_NONE,
		.OverSampling = LL_USART_OVERSAMPLING_8
	};
	LL_DMA_InitTypeDef dma2_usart1_config = {
		.PeriphOrM2MSrcAddress = LL_USART_DMA_GetRegAddr(USART1),
		.MemoryOrM2MDstAddress = 0, // That should be changed later
		.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH,
		.Mode = LL_DMA_MODE_NORMAL,
		.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT,
		.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT,
		.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE,
		.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE,
		.NbData = 0, // We'll should change that later
		.Channel = LL_DMA_CHANNEL_4, // stream 7 but channel 4
		.Priority = LL_DMA_PRIORITY_HIGH,
		.FIFOMode = LL_DMA_FIFOMODE_ENABLE,
		.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_3_4,
		.MemBurst = LL_DMA_PBURST_SINGLE,
		.PeriphBurst = LL_DMA_PBURST_SINGLE
	};
	/*  TODO try disable GPIOA after initialisation */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);
	LL_GPIO_Init(GPIOA, &usart1_gpio);
	LL_USART_Init(USART1, &usart1_config);
	LL_DMA_Init(DMA2, LL_DMA_STREAM_7, &dma2_usart1_config);
	LL_USART_Enable(USART1);
	LL_USART_EnableDMAReq_TX(USART1);
	LL_USART_EnableIT_RXNE(USART1);
	LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_7);
}
extern EventGroupHandle_t ev1;
extern SemaphoreHandle_t usart1_tx_mutex;

/* buf should be located on SRAM, use keyword static, num is number of bytes */
void usart1_tx(const char *buf, const uint32_t num)
{
	/* hold mutex */
	if (num == 0) {
		return;
	}
	xSemaphoreTake(usart1_tx_mutex, portMAX_DELAY);
	LL_USART_ClearFlag_TC(USART1);
	LL_DMA_SetM2MDstAddress(DMA2, LL_DMA_STREAM_7, (uint32_t) buf);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_7, num);
	LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_7);
	/* wait for transmition to complete */
	xEventGroupWaitBits(ev1, USART1_TX_SEM_BIT,
				 pdTRUE, pdTRUE, portMAX_DELAY);
	/* release mutex */
	xSemaphoreGive(usart1_tx_mutex);
}

/*  usart1 tx irq */
void DMA2_Stream7_IRQHandler(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	LL_DMA_ClearFlag_TC7(DMA2);
	LL_DMA_DisableStream(DMA2, LL_DMA_STREAM_7);
	xEventGroupSetBitsFromISR(ev1, USART1_TX_SEM_BIT,
				&xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

extern SemaphoreHandle_t tty_mutex;
extern QueueHandle_t usart1_rx_queue;
extern TaskHandle_t leo_tsk_hdl;
extern char user_input[64];
void TTY_Task(void *pvParameters)
{
	uint8_t cnt = 0;
	usart1_tx("tty task is here!\r\n", 19);
	while(1) {
		xSemaphoreTake(tty_mutex, portMAX_DELAY);
		/* priority of leonardo task should be higher */
		xTaskNotifyGive(leo_tsk_hdl);
		while (1) {
			xQueueReceive(usart1_rx_queue, &user_input[cnt], portMAX_DELAY);
			usart1_tx(&user_input[cnt], 1); // echo char back
			if ('\b' == user_input[cnt]) {
				/* we *need* nested if here */
				if (cnt > 0) {
					user_input[cnt - 1] = ' ';
					user_input[cnt] = '\0';
					/* not atomic a bit, but not lethal either */
					usart1_tx("\r", 1);
					usart1_tx(user_input, cnt);
					--cnt;
				}
			}
			else if ('\r' == user_input[cnt]) {
				usart1_tx("\n", 1); // put LF
				user_input[cnt] = '\0';
				cnt = 0;
				xSemaphoreGive(tty_mutex);
				break;
			} else {
				++cnt;
			}
		}

	}
}

void USART1_IRQHandler(void)
{
	char byte;
	if (LL_USART_IsActiveFlag_RXNE(USART1)) {
		byte = LL_USART_ReceiveData8(USART1);
		LL_GPIO_TogglePin(GPIOE, LL_GPIO_PIN_13);
		xQueueSendFromISR(usart1_rx_queue, &byte, pdFALSE);
	}
}
