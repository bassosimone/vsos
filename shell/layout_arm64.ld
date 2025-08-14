/* File: shell/layour_arm64.ld
   Purpose: loader script for the shell
   SPDX-License-Identifier: MIT
   Adapted from: https://github.com/nuta/operating-system-in-1000-lines */
ENTRY(_start)

SECTIONS {
    . = 0x1000000;

    /* machine code */
    .text : ALIGN(4096) {
        KEEP(*(.text.start));
        *(.text .text.*);
    }

    /* read-only data */
    .rodata : ALIGN(4096) {
        *(.rodata .rodata.*);
    }

    /* data with initial values */
    .data : ALIGN(4096) {
        *(.data .data.*);
    }

    /* data that should be zero-filled at startup */
    .bss : ALIGN(4096) {
        *(.bss .bss.* .sbss .sbss.*);
    }
}
