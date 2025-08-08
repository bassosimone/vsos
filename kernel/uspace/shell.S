    // File: kernel/uspace/shell.S
    // Purpose: include the shell as a raw binary into the kernel
    // SPDX-License-Identifier: MIT

    .section .rodata
    .global _shell_bin, _shell_bin_end
_shell_bin:
    .incbin "shell.bin"
_shell_bin_end:
