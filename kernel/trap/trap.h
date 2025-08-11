// File: kernel/trap/trap.h
// Purpose: Trap management subsystem
// SPDX-License-Identifier: MIT
#ifndef KERNEL_TRAP_TRAP_H
#define KERNEL_TRAP_TRAP_H

#include <sys/types.h>

// Initialize the memory mapping for trap handling.
//
// Called by the mm subsystem.
//
// This needs to happen before trap_init_irqs.
void trap_init_mm(void);

// Initialize interrupt handling.
//
// Called by the boot process.
//
// Must be invoked after trap_init_mm.
void trap_init_irqs(void);

// Function that returns from the trap restoring the previous context.
//
// Called by the scheduler to switch the context.
//
// The frame variable points to the MD trap frame context.
[[noreturn]] void trap_restore_user_and_eret(uintptr_t frame);

#endif // KERNEL_TRAP_TRAP_H
