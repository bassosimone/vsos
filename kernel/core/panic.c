// File: kernel/core/panic.c
// Purpose: implementation definition of panic.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/core/panic.h>
#include <kernel/core/printk.h>

[[noreturn]] void panic(const char *fmt, ...) {
	__builtin_va_list ap;
	__builtin_va_start(ap, fmt);
	vprintk(fmt, ap);
	__builtin_va_end(ap);
}
