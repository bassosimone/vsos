// File: kernel/exec/layout.h
// Purpose: Memory layour for user programs.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_EXEC_LAYOUT_H
#define KERNEL_EXEC_LAYOUT_H

#include <sys/types.h> // for size_t

// The user program base as defined in the linker script.
#define LAYOUT_USER_PROGRAM_BASE 0x1000000

// The arbitrary maximum size of the user program.
#define LAYOUT_MAX_USER_PROGRAM_SIZE 0x800000

// The limit starting at which we cannot have user stuff.
#define LAYOUT_USER_PROGRAM_LIMIT (LAYOUT_USER_PROGRAM_BASE + LAYOUT_MAX_USER_PROGRAM_SIZE)

// The bottom of the user program stack.
#define LAYOUT_USER_STACK_BOTTOM 0x2000000

// The top of the user program stack.
#define LAYOUT_USER_STACK_TOP 0x2040000

// We do not enforce a maximum file size in the linker script but we
// check here that the user program is within bounds.
static inline bool layout_valid_virtual_address(uintptr_t candidate) {
	return candidate >= LAYOUT_USER_PROGRAM_BASE && candidate < LAYOUT_USER_PROGRAM_LIMIT;
}

// Ensures that a virtual address plus its offset is still valid virtual memory.
static inline bool layout_valid_virtual_address_offset(uintptr_t base, uintptr_t off) {
	if (!layout_valid_virtual_address(base)) {
		return false;
	}
	if (base > UINTPTR_MAX - off) {
		return false;
	}
	return layout_valid_virtual_address(base + off);
}

#endif // KERNEL_EXEC_LAYOUT_H
