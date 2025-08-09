// File: kernel/sys/syscalls_arm64.c
// Purpose: system calls
// SPDX-License-Identifier: MIT

#include <kernel/irq/arm64.h>	// for struct trapframe
#include <kernel/sys/errno.h>	// for EBADF
#include <kernel/sys/syscall.h> // for SYS_write
#include <kernel/sys/types.h>	// for uint64_t
#include <kernel/tty/uart.h>	// for uart_read

// Returns the syscall from the x8 register just like on Linux
static inline uint64_t sysno_from_regs(struct trapframe *frame) {
	return frame->x[8];
}

// Return the first syscall argument from the registers
static inline uint64_t arg0(struct trapframe *frame) {
	return frame->x[0];
}

// Return the second syscall argument from the registers
static inline uint64_t arg1(struct trapframe *frame) {
	return frame->x[1];
}

// Return the third syscall argument from the registers
static inline uint64_t arg2(struct trapframe *frame) {
	return frame->x[2];
}

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
		return uart_read(buffer, siz);

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
		return uart_write(buffer, siz);

	default:
		return (uint64_t)-EBADF;
	}
}

void __syscall_handle(uint64_t rframe, uint64_t esr, uint64_t far) {
	// TODO(bassosimone): we should use these
	(void)esr;
	(void)far;

	struct trapframe *frame = (struct trapframe *)rframe;
	switch (sysno_from_regs(frame)) {
	case SYS_read:
		frame->x[0] = sys_read(arg0(frame), arg1(frame), arg2(frame));
		break;

	case SYS_write:
		frame->x[0] = sys_write(arg0(frame), arg1(frame), arg2(frame));
		break;

	default:
		frame->x[0] = (uint64_t)-ENOSYS;
		break;
	}
}
