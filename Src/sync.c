/* This file is used for synchronisation and intertask communication,
 * i.e. mutexes, semaphores, event groups, queues stuff, created on init */
#include "main.h"

/* all syncronisation entities declared in main.h */

EventGroupHandle_t ev1;
SemaphoreHandle_t usart1_tx_mutex;
SemaphoreHandle_t usart1_rx_mutex;

void syncronisation_init(void)
{
	ev1 = xEventGroupCreate();
	usart1_tx_mutex = xSemaphoreCreateMutex();
	usart1_rx_mutex = xSemaphoreCreateMutex();
}
