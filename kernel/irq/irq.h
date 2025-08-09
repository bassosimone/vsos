// File: kernel/irq/irq.h
// Purpose: IRQ public and protected interface
// SPDX-License-Identifier: MIT
#ifndef KERNEL_IRQ_IRQ_H
#define KERNEL_IRQ_IRQ_H

#include <kernel/sys/types.h>

// Initialize interrupt handling.
//
// Invoked by the kernel boot process.
void irq_init(void);

// Handles interrupts.
//
// Invoked by the interrupt handler written in assembly.
void irq_handle(uintptr_t frame);

// Function that returns from the interrupt restoring the context.
//
// Invoked by the scheduler to switch the context.
[[noreturn]] void irq_restore_user_and_eret(uintptr_t frame);

#endif // KERNEL_IRQ_IRQ_H
