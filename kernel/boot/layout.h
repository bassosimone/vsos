// File: kernel/boot/layout.h
// Purpose: memory layout available to the kernel
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_BOOT_LAYOUT_H
#define KERNEL_BOOT_LAYOUT_H

// Start of kernel code
extern char __kernel_base[];

// Start of BSS section
extern char __bss[];

// End of BSS section
extern char __bss_end[];

// Top of kernel stack
extern char __stack_top[];

// Start of free memory
extern char __free_ram[];

// End of free memory
extern char __free_ram_end[];

#endif // KERNEL_BOOT_LAYOUT_H
