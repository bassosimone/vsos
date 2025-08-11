// File: kernel/tty/uart.c
// Purpose: MI UART code
// SPDX-License-Identifier: MIT

#include <kernel/core/assert.h> // for KERNEL_ASSERT
#include <kernel/tty/uart.h>	// for uart_read

#include <sys/errno.h> // for EWOULDBLOCK

ssize_t uart_read(char *buffer, size_t siz) {
	return uart_recv(buffer, siz, /* flags */ 0);
}

ssize_t uart_write(const char *buffer, size_t siz) {
	return uart_send(buffer, siz, /* flags */ 0);
}
