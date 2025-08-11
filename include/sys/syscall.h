// File: include/sys/syscall.h
// Purpose: system calls
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef __SYS_SYSCALL_H__
#define __SYS_SYSCALL_H__

// We try to use the same numbers used by Linux on x86_64

// The read(1) system call
#define SYS_read 0

// The write(1) system call
#define SYS_write 1

#endif // __SYS_SYSCALL_H__
