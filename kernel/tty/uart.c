// File: kernel/tty/uart.c
// Purpose: MI UART code
// SPDX-License-Identifier: MIT

#include <kernel/core/assert.h> // for KERNEL_ASSERT
#include <kernel/sys/errno.h>	// for EWOULDBLOCK
#include <kernel/tty/uart.h>	// for uart_read

ssize_t uart_read(char *buffer, size_t siz) {
	// Make sure we don't overflow the return value
	siz = (siz <= SSIZE_MAX) ? siz : SSIZE_MAX;
	size_t nb = 0;

	// Read as many bytes as possible
	while (nb < siz) {
		int16_t res = uart_try_read();
		if (res == -EWOULDBLOCK) {
			// TODO(bassosimone): block the thread
			continue;
		}
		if (res < 0) {
			return (nb > 0) ? nb : (ssize_t)res;
		}
		buffer[nb++] = (char)res;
	}

	// Return the result
	return nb;
}

ssize_t uart_write(const char *buffer, size_t siz) {
	// Make sure we don't overflow the return value
	siz = (siz <= SSIZE_MAX) ? siz : SSIZE_MAX;
	size_t nb = 0;

	// Write as many bytes as possible
	while (nb < siz) {
		int16_t res = uart_try_write(buffer[nb]);
		if (res == -EWOULDBLOCK) {
			// TODO(bassosimone): block the thread
			continue;
		}
		if (res < 0) {
			return (nb > 0) ? nb : (ssize_t)res;
		}
		KERNEL_ASSERT(res == 0);
		nb++;
	}

	// Return the result
	return nb;
}
