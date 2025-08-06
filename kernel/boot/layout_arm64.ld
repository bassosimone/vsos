/* File: kernel/boot/layout_arm64.ld
   Purpose: Linker script for the ARM64 port.
   SPDX-License-Identifier: MIT
   Adapted from: https://github.com/nuta/operating-system-in-1000-lines */

/* The kernel entry point is the function named `boot`. */
ENTRY(boot)

SECTIONS {
    /* Load address: match QEMU's default load address for `-kernel kernel.bin` */
    . = 0x40080000;
    __kernel_base = .;

    /* Code section: contains kernel code, including the `boot()` entry point.
       `KEEP` ensures `.text.boot` is retained even if unreferenced. */
    .text : {
        KEEP(*(.text.boot));
        *(.text .text.*);
    }

    /* Read-only data section: constants, jump tables, etc.
       16-byte aligned for safety on AArch64 (e.g., 64-bit or vector constants) */
    .rodata : ALIGN(16) {
        *(.rodata .rodata.*);
    }

    /* Initialized data: global variables with initial values
       16-byte aligned for pointer safety on ARM64 */
    .data : ALIGN(16) {
        *(.data .data.*);
    }

    /* Uninitialized data (BSS): zero-filled at runtime in `boot()`.
       Provides __bss and __bss_end for manual zeroing. */
    .bss : ALIGN(16) {
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*);
        __bss_end = .;
    }

    /* Stack space: 128 KiB for kernel stack
       __stack_top marks the "top" (lowest address grows downward) */
    . = ALIGN(16);
    . += 128 * 1024;
    __stack_top = .;

    /* Free RAM: 64 MiB of contiguous memory for dynamic page allocations.
       This is identity-mapped and managed by the kernel allocator.
       Must be 4 KiB aligned for paging. */
    . = ALIGN(4096);
    __free_ram = .;
    . += 64 * 1024 * 1024;
    __free_ram_end = .;
}
