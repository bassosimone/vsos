// File: kernel/core/panic.c
// Purpose: implementation definition of panic.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/asm/asm.h>	// for cpu_sleep_until_interrupt
#include <kernel/core/panic.h>	// for panic
#include <kernel/core/printk.h> // for printk

[[noreturn]] void panic(const char *fmt, ...) {
	// Hopefully print a message on the console
	__builtin_va_list ap;
	__builtin_va_start(ap, fmt);
	vprintk(fmt, ap);
	__builtin_va_end(ap);

	// Start spinning very very very slowly
	for (;;) {
		cpu_sleep_until_interrupt();
	}
}
