    // File: kernel/uspace/shell.S
    // Purpose: include the shell as a raw binary into the kernel
    // SPDX-License-Identifier: MIT

    .section .rodata
    .global __shell_start, __shell_end
__shell_start:
    .incbin "shell.elf"
__shell_end:
