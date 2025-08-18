// File: kernel/sys/write.c
// Purpose: implement the write syscall
// SPDX-License-Identifier: MIT

#include <kernel/syscall/io.h> // for copy_from_user
#include <kernel/tty/uart.h>   // for uart_send

#include <sys/errno.h> // for EBADF
#include <sys/types.h> // for size_t

#include <unistd.h> // for write

ssize_t write(int fd, const char *user_buf, size_t count) {
	// Avoid overflowing ssize_t
	if (count > SSIZE_MAX) {
		count = SSIZE_MAX;
	}

	// We copy inside a kernel buffer and then write the kernel buffer
	char kernel_buf[128];
	count = (count < sizeof(kernel_buf)) ? count : sizeof(kernel_buf);
	ssize_t rv = copy_from_user(kernel_buf, user_buf, count);
	if (rv <= 0) {
		return rv;
	}
	count = rv;

	switch (fd) {
	case 0:
	case 1:
	case 2:
		return uart_send(kernel_buf, count, /* flags */ 0);

	default:
		return -EBADF;
	}
}
