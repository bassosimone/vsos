// File: kernel/sys/read.c
// Purpose: implement the read syscall
// SPDX-License-Identifier: MIT

#include <kernel/tty/uart.h> // for uart_recv

#include <sys/errno.h> // for EBADF
#include <sys/types.h> // for size_t

#include <unistd.h> // for syscall

// Implement the read system call.
ssize_t read(int fd, char *buf, size_t count) {
	// TODO(bassosimone): copy the buffers from/to kernel space.

	switch (fd) {
	case 0:
	case 1:
	case 2:
		return uart_recv(buf, count, /* flags*/ 0);

	default:
		return (uint64_t)-EBADF;
	}
}
