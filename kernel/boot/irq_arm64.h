// File: kernel/boot/irq_arm64.h
// Purpose: interface to manage IRQs on arm64
// SPDX-License-Identifier: MIT
#ifndef KERNEL_BOOT_IRQ_ARM64_H
#define KERNEL_BOOT_IRQ_ARM64_H

// Init the Generic Interrupt Controller v2 and enable interrupts.
void gicv2_init();

// Handles exceptions at execution level 1 (kernel) using a dedicated stack.
void irq_dispatch_el1h(void);

#endif // KERNEL_BOOT_IRQ_ARM64_H
