// File: kernel/core/panic_arm64.c
// Purpose: ARM64 panic implementation
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/asm/arm64.h>
#include <kernel/core/panic.h>

[[noreturn]] void __panic_halt(void) {
	for (;;) {
		wfi();
	}
}
