// File: kernel/sys/syscalls_arm64.c
// Purpose: system calls
// SPDX-License-Identifier: MIT

#include <kernel/tty/uart.h> // for uart_read

#include <sys/errno.h>	 // for EBADF
#include <sys/syscall.h> // for SYS_write
#include <sys/types.h>	 // for uint64_t

#include <unistd.h> // for syscall

// Implement the read system call.
static uint64_t sys_read(uint64_t a0, uint64_t a1, uint64_t a2) {
	int fd = (int)a0;
	char *buffer = (char *)a1;
	size_t siz = (size_t)a2;

	// TODO(bassosimone): copy the buffers from/to kernel space.

	switch (fd) {
	case 0:
	case 1:
	case 2:
		return uart_recv(buffer, siz, /* flags*/ 0);

	default:
		return (uint64_t)-EBADF;
	}
}

// Implement the write system call.
static uint64_t sys_write(uint64_t a0, uint64_t a1, uint64_t a2) {
	int fd = (int)a0;
	const char *buffer = (char *)a1;
	size_t siz = (size_t)a2;

	// TODO(bassosimone): copy the buffers from/to kernel space.

	switch (fd) {
	case 0:
	case 1:
	case 2:
		return uart_send(buffer, siz, /* flags */ 0);

	default:
		return (uint64_t)-EBADF;
	}
}

intptr_t
syscall(uintptr_t num, uintptr_t a0, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5) {
	(void)a3;
	(void)a4;
	(void)a5;

	switch (num) {
	case SYS_read:
		return sys_read(a0, a1, a2);

	case SYS_write:
		return sys_write(a0, a1, a2);

	default:
		return -ENOSYS;
	}
}
