#include "main.h"
#include <ctype.h>

/* simple tty driver over uart.
 * everything is simple for tx - just push n bytes out
 * but for rx we should buffer input until \n is encountered
 */

static void TTY_Task(void *pvParameters);

static struct tty_driver {
        /* tx mutex */
        SemaphoreHandle_t lock_tx;
        /* tx semaphore */
        SemaphoreHandle_t sem_tx;
        /* rx mutex */
        SemaphoreHandle_t lock_rx;
        /* rx semaphore */
        SemaphoreHandle_t sem_rx;

        QueueHandle_t q_rx;
        char *tx_buf;
        size_t tx_num;
        char *rx_buf;
        size_t rx_num;
	bool rx_enable;
} tty;

static void usart1_init(void)
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

void tty_driver_init()
{
        tty.lock_tx = xSemaphoreCreateMutex();
        tty.lock_rx = xSemaphoreCreateMutex();
        tty.sem_tx = xSemaphoreCreateBinary();
        tty.sem_rx = xSemaphoreCreateBinary();
        tty.q_rx = xQueueCreate(16, sizeof(char));
	tty.rx_enable = false;
        usart1_init();
        xTaskCreate(TTY_Task, "tty", 256, NULL, TTY_TASK_PRIORITY, NULL);
}

size_t tty_driver_tx(char *ptr, size_t num)
{
        if (num <= 0) {
                return 0;
        }
        xSemaphoreTake(tty.lock_tx, portMAX_DELAY);
        tty.tx_num = num;
        tty.tx_buf = ptr;
        LL_USART_EnableIT_TXE(USART1);
        /* wait until buffer is transferred */
        xSemaphoreTake(tty.sem_tx, portMAX_DELAY);
        xSemaphoreGive(tty.lock_tx);
        return tty.tx_num;
}

size_t tty_driver_rx(char *buf, size_t num)
{
        if (num <= 0) {
                return 0;
        }
        xSemaphoreTake(tty.lock_rx, portMAX_DELAY);
        tty.rx_buf = buf;
        tty.rx_num = num;
	tty.rx_enable = true;
        /* wait until TTY_Task ends reception */
        xSemaphoreTake(tty.sem_rx, portMAX_DELAY);
        xSemaphoreGive(tty.lock_rx);
        return tty.rx_num;
}

void USART1_IRQHandler(void)
{
	char byte;
        static uint32_t tx_ind = 0;
        BaseType_t xHigherPriorityTaskWoken;

        if (LL_USART_IsActiveFlag_TXE(USART1) &&
                                        LL_USART_IsEnabledIT_TXE(USART1)) {
                LL_USART_TransmitData8(USART1, tty.tx_buf[tx_ind++]);
                if (tx_ind == tty.tx_num) {
                        tx_ind = 0;
                        LL_USART_DisableIT_TXE(USART1);
                        xSemaphoreGiveFromISR(tty.sem_tx,
                                        &xHigherPriorityTaskWoken);
                        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                }
                return;
        }
        if (LL_USART_IsActiveFlag_RXNE(USART1) &&
                                        LL_USART_IsEnabledIT_RXNE(USART1)) {
		byte = LL_USART_ReceiveData8(USART1);
		xQueueSendFromISR(tty.q_rx, &byte, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                return;
	}
}

/* This function imitates tty to be used in read() syscall of newlib C
 * When acquiring semaphore it reads up to tty.rx_num bytes or until '\r' encountered
 */
/* maybe someday i'll make support for escape sequences */
static void TTY_Task(void *pvParameters)
{
	uint8_t cnt = 0;
        char byte;
	LL_USART_EnableIT_RXNE(USART1);
        printf("tty task is here!\r\n");
	while(1) {
		while (1) {
			xQueueReceive(tty.q_rx, &byte, portMAX_DELAY);
                        /* echoing back! */
                        tty_driver_tx(&byte, 1);
			if (!tty.rx_enable)
				continue;
                        if ('\r' == byte) { /* enter pressed */
                                tty.rx_buf[cnt++] = '\n';
                                printf("\r\n");
                                break;
			}
			/* minicom sends '\b', picocom sends 0x7f */
                        else if ('\b' == byte || byte == 0x7f) { /* backspace */
				if (cnt > 0) {
					tty.rx_buf[--cnt] = '\0';
                                        printf("\r%s \b", tty.rx_buf);
                                        fflush(stdout);
				}
			}
                        else if (byte >= 0x20 && byte <= 0x7E) { /* is printable */
                                tty.rx_buf[cnt++] = byte;
                                if (cnt == tty.rx_num) {
                                        break;
                                }
                        }
                        else {
                                /* exhausting pattern mathing you know ... */
                                printf("Unknown Symbol %x\r\n", byte);
                                break;
                        }
		}
                tty.rx_num = cnt;
		tty.rx_enable = false;
                cnt = 0;
                xSemaphoreGive(tty.sem_rx);
	}
}
