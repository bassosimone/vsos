// File: kernel/sys/write.c
// Purpose: implement the write syscall
// SPDX-License-Identifier: MIT

#include <kernel/tty/uart.h> // for uart_send

#include <sys/errno.h> // for EBADF
#include <sys/types.h> // for size_t

#include <unistd.h> // for syscall

ssize_t write(int fd, const char *buf, size_t count) {
	// TODO(bassosimone): copy the buffers from/to kernel space.

	switch (fd) {
	case 0:
	case 1:
	case 2:
		return uart_send(buf, count, /* flags */ 0);

	default:
		return (uint64_t)-EBADF;
	}
}
