// File: kernel/core/panic_amd64.c
// Purpose: AMD64 panic implementation
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/asm/amd64.h>
#include <kernel/core/panic.h>

[[noreturn]] void __panic_halt(void) {
	for (;;) {
		hlt();
	}
}
