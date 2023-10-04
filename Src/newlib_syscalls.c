#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include "newlib_syscalls.h"
/*  _sbrk() specific for stm32f407vet mcu
 *  heap is allocated on CCMRAM just for my fun
 * _impure_ptr
 */
/* BaseType_t is uint32_t on armv8m */
struct stat;
extern uint32_t xTaskResumeAll(void);
extern void vTaskSuspendAll(void);
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

int _write (int fd, const void *buf, size_t nbyte)
{
	(void) fd;
	(void) buf;
	(void) nbyte;
	return -1;
}

int _close (int fildes)
{
	(void) fildes;
	return -1;
}

long _lseek (int fildes, long offset, int whence)
{
	(void) fildes;
	(void) offset;
	(void) whence;
	return -1;
}

int _read (int fd, void *buf, size_t nbyte)
{
	(void) fd;
	(void) buf;
	(void) nbyte;
	return -1;
}
int _fstat (int fd, struct stat *sbuf)
{
	(void) fd;
	(void) sbuf;
	return -1;
}

int _isatty (int fildes)
{
	(void) fildes;
	return -1;
}
