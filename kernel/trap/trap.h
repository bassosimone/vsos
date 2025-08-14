// File: kernel/trap/trap.h
// Purpose: Trap management subsystem
// SPDX-License-Identifier: MIT
#ifndef KERNEL_TRAP_TRAP_H
#define KERNEL_TRAP_TRAP_H

#include <kernel/mm/vm.h> // for struct vm_root_pt

#include <sys/types.h> // for uintptr_t

// Initialize the data structures required to handling traps.
//
// Must be called first by early boot.
void trap_init_early(void);

// Initialize the memory mapping for trap handling.
//
// Called by the mm subsystem.
//
// This needs to happen before trap_init_irqs.
void trap_init_mm(struct vm_root_pt root);

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

// Creates a trap_frame on the stack that fakes out the invocation of
// a system call so that ERET will bring us back to userspace.
//
// Returns the pointer to the beginning of the frame inside the stack
// so that it's then possible to call trap_restore_user_and_eret.
uintptr_t trap_create_process_frame(uintptr_t entry, uintptr_t pg_table, uintptr_t stack_top);

#endif // KERNEL_TRAP_TRAP_H
