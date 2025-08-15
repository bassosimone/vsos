// File: kernel/sys/read.c
// Purpose: implement the read syscall
// SPDX-License-Identifier: MIT

#include <kernel/syscall/io.h> // for copy_to_user
#include <kernel/tty/uart.h>   // for uart_recv

#include <sys/errno.h> // for EBADF
#include <sys/types.h> // for size_t

#include <unistd.h> // for read

// Implement the read system call.
ssize_t read(int fd, char *user_buf, size_t count) {
	// Avoid overflowing ssize_t
	if (count > SSIZE_MAX) {
		count = SSIZE_MAX;
	}

	// We read inside a kernel buffer and then copy back to user
	char kernel_buf[128];
	count = (count < sizeof(kernel_buf)) ? count : sizeof(kernel_buf);

	// Perform the actual read from the device
	ssize_t rv = 0;
	switch (fd) {
	case 0:
	case 1:
	case 2:
		rv = uart_recv(kernel_buf, count, /* flags*/ 0);
		break;

	default:
		rv = -EBADF;
		break;
	}

	// Handle the failure case.
	if (rv < 0) {
		return rv;
	}

	// Copy data back to userspace
	return copy_to_user(user_buf, kernel_buf, rv);
}
