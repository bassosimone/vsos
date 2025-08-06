// File: kernel/core/panic.h
// Purpose: portable definition of panic.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_CORE_PANIC_H
#define KERNEL_CORE_PANIC_H

[[noreturn]] void panic(const char *fmt, ...);

#endif // KERNEL_CORE_PANIC_H
