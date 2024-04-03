#include "main.h"

/* driver for AT24C02BN i2c flash, 256 bytes */

static struct i2c_flash {
	const void *tx_buf;
	void *rx_buf;
	SemaphoreHandle_t lock;
	SemaphoreHandle_t smph;
	uint8_t tx_len;
	uint8_t rx_len;
} i2c_flash;

int i2c_flash_init()
{
	LL_GPIO_InitTypeDef i2c1_pins = {
		.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9,
		.Mode = LL_GPIO_MODE_ALTERNATE,
		.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH,
		.OutputType = LL_GPIO_OUTPUT_PUSHPULL,
		.Pull = LL_GPIO_PULL_NO,
		.Alternate = LL_GPIO_AF_4 // I2C1
	};
	LL_I2C_InitTypeDef i2c1_dev = {
		.PeripheralMode = LL_I2C_MODE_I2C,
		.ClockSpeed = 100000, // 100 kHz
		.DutyCycle = LL_I2C_DUTYCYCLE_2,
		.OwnAddress1 = 0x0,
		.TypeAcknowledge = LL_I2C_NACK,
		.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT
	};
	i2c_flash.lock = xSemaphoreCreateMutex();
	i2c_flash.smph = xSemaphoreCreateBinary();
	__NVIC_EnableIRQ(I2C1_EV_IRQn);
	NVIC_SetPriority(I2C1_EV_IRQn,
		NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 10, 0));

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	LL_GPIO_Init(GPIOB, &i2c1_pins);
	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
	LL_I2C_Init(I2C1, &i2c1_dev);
	return 0;
}

int i2c_flash_write(const void *buf, uint8_t len)
{
	xSemaphoreTake(i2c_flash.lock, portMAX_DELAY);
	i2c_flash.tx_buf = buf;
	i2c_flash.tx_len = len;

	/* wait until transfer all the bytes */
	xSemaphoreTake(i2c_flash.smph, portMAX_DELAY);

	/* and release the lock */
	xSemaphoreGive(i2c_flash.lock);
	return 0;
}

int i2c_flash_read(void *buf, uint8_t len)
{

	return 0;
}

void I2C1_EV_IRQHandler(void) {

}

/*
void I2C1_ER_IRQHandler(void) {

}
*/
