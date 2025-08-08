// File: kernel/boot/irq.h
// Purpose: MI interrupt code
// SPDX-License-Identifier: MIT
#ifndef KERNEL_BOOT_IRQ_H
#define KERNEL_BOOT_IRQ_H

#include <kernel/sys/types.h>

// Function that returns from the interrupt restoring the context.
[[noreturn]] void irq_restore_user_and_eret(uintptr_t frame);

#endif // KERNEL_BOOT_IRQ_H
