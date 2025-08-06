// File: kernel/core/panic.h
// Purpose: portable definition of panic.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_CORE_PANIC_H
#define KERNEL_CORE_PANIC_H

// Prints the given message and halts the kernel
[[noreturn]] void panic(const char *fmt, ...);

// Machine-dependent implementation of halting the kernel
[[noreturn]] void __panic_halt(void);

#endif // KERNEL_CORE_PANIC_H
