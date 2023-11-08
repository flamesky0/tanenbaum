#ifndef USART1_DRIVER
#define USART1_DRIVER

void tty_driver_init();

/* transmit num bytes from buffer ptr
 * returns number of transmitted bytes
 * */
size_t tty_driver_tx(char *ptr, size_t num);

/* receive up to num bytes to buffer buf,
 * input line-buffered
 * returs number of received bytes */
size_t tty_driver_rx(char *buf, size_t num);
#endif /*  USART1_DRIVER */
