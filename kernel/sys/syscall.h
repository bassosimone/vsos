// File: kernel/sys/syscall.h
// Purpose: system calls
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_SYS_SYSCALL_H
#define KERNEL_SYS_SYSCALL_H

#include <kernel/sys/types.h>

// We try to use the same numbers used by Linux on x86_64

// The read(1) system call
#define SYS_read 0

// The write(1) system call
#define SYS_write 1

// Kernel internal function invoked to handle syscalls.
void __syscall_handle(uint64_t rframe, uint64_t esr, uint64_t far);

#endif // KERNEL_SYS_SYSCALL_H
