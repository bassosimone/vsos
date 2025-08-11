// File: kernel/irq/irq.h
// Purpose: IRQ public and protected interface
// SPDX-License-Identifier: MIT
#ifndef KERNEL_IRQ_IRQ_H
#define KERNEL_IRQ_IRQ_H

#include <sys/types.h>

// Initialize the memory mapping for interrupt handling.
//
// Called by the mm subsystem.
//
// This needs to happen before irq_init.
void irq_init_mm(void);

// Initialize interrupt handling.
//
// Called by the kernel boot process after mm_init.
void irq_init(void);

// Handles interrupts.
//
// Called by the interrupt handler written in assembly.
void irq_handle(uintptr_t frame);

// Function that returns from the interrupt restoring the context.
//
// Called by the scheduler to switch the context.
[[noreturn]] void irq_restore_user_and_eret(uintptr_t frame);

#endif // KERNEL_IRQ_IRQ_H
