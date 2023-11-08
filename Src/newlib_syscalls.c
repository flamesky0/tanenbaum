#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <FreeRTOS.h>
#include "list.h"
#include "queue.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#include "usart1_driver.h"


/*  _sbrk() specific for stm32f407vet mcu
 *  heap is allocated on CCMRAM just for my fun
 * _impure_ptr
 */
/* BaseType_t is uint32_t on armv8m */
struct stat;
// extern uint32_t xTaskResumeAll(void);
// extern void vTaskSuspendAll(void);
uint8_t *_sbrk(int incr)
{
#define HEAP_SIZE 64 * 1024 /*  size of CCM memory, 64 KB */

	static uint8_t heap[HEAP_SIZE]
		__attribute__((section(".ccmram")));
	static uint8_t *heap_end = heap; /*  initial value */
	uint8_t *prev_heap_end;
	vTaskSuspendAll();
	prev_heap_end = heap_end;
	if (heap_end + incr > heap + HEAP_SIZE) {
		errno = ENOMEM;
		return (uint8_t *) -1;
	}
	heap_end += incr;
	xTaskResumeAll();
	return prev_heap_end;
}

extern SemaphoreHandle_t usart1_tx_mutex;
int _write (int fd, void *buf, size_t nbytes)
{
	(void) fd;
        return tty_driver_tx(buf, nbytes);
}

int _read (int fd, void *buf, size_t nbytes)
{
	(void) fd;
	return tty_driver_rx(buf, nbytes);
}

int _close (int fildes)
{
	(void) fildes;
	/*  always succeedes */
	return 0;
}

long _lseek (int fildes, long offset, int whence)
{
	(void) fildes;
	(void) offset;
	(void) whence;
	return 0;
}


int _fstat (int fd, struct stat *sbuf)
{
	(void) fd;
	(void) sbuf;
	//st->st_mode = S_IFCHR;
	return 0;
}

int _isatty (int fildes)
{
	(void) fildes;
	return 1;
}
