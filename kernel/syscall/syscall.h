// File: kernel/syscall/syscall.h
// Purpose: The kernel system calls
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_SYSCALL_SYSCALL_H
#define KERNEL_SYSCALL_SYSCALL_H

#include <sys/syscall.h>
#include <sys/types.h>

// Kernel internal function invoked to handle syscalls.
void __syscall_handle(uint64_t rframe, uint64_t esr, uint64_t far);

#endif // KERNEL_SYSCALL_SYSCALL_H
