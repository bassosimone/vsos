// File: kernel/boot/boot.h
// Purpose: boot subsystem interface
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_BOOT_BOOT_H
#define KERNEL_BOOT_BOOT_H

// Start of kernel code
extern char __kernel_base[];

// End of kernel code
extern char __kernel_end[];

// Start of rodata
extern char __rodata_base[];

// End of rodata
extern char __rodata_end[];

// Start of data
extern char __data_base[];

// End of data
extern char __data_end[];

// Start of BSS section
extern char __bss[];
#define __bss_base __bss

// End of BSS section
extern char __bss_end[];

// Bottom of kernel stack
extern char __stack_bottom[];

// Top of kernel stack
extern char __stack_top[];

// Start of free memory
extern char __free_ram[];

// End of free memory
extern char __free_ram_end[];

// The initial naked function invoked by the bootloader.
__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void);

// The machine dependent initialization function.
[[noreturn]] void kernel_main(void);

// The machine independent initialization function.
[[noreturn]] void __kernel_main(void);

#endif // KERNEL_BOOT_BOOT_H
