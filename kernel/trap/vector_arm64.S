    // File: kernel/trap/vector_arm64.S
    // Purpose: ARM64 trap vectors definition
    // SPDX-License-Identifier: MIT

    .section .text.vectors, "ax", %progbits
    .align 11                        // 2KB alignment for VBAR_EL1
    .global __vectors_el1
    .type   __vectors_el1, %function
__vectors_el1:
    // 0x000: Synchronous EL1t (current EL, SP0)
    .balign 128
    b .

    // 0x080: IRQ EL1t
    .balign 128
    b .

    // 0x100: FIQ EL1t
    .balign 128
    b .

    // 0x180: SError EL1t
    .balign 128
    b .

    // 0x200: Synchronous EL1h (current EL, SPx)
    .balign 128
    b .

    // 0x280: IRQ EL1h
    .balign 128
    b __trap_handle_el1h_irq

    // 0x300: FIQ EL1h
    .balign 128
    b .

    // 0x380: SError EL1h
    .balign 128
    b .

    // 0x400: Synchronous from lower EL using AArch64 (EL0 64-bit)
    .balign 128
    b __trap_handle_synchronous

    // 0x480: IRQ from lower EL using AArch64
    // TODO(bassosimone): this is technical debt. However, for now it
    // is fine to reuse the same code. We lose the ability to reschedule
    // when an interrupt happens in user code. Also, the function must
    // save everything, and it could be leaner for the kernel.
    .balign 128
    b __trap_handle_el1h_irq

    // 0x500: FIQ from lower EL using AArch64
    .balign 128
    b .

    // 0x580: SError from lower EL using AArch64
    .balign 128
    b .

    // 0x600: Synchronous from lower EL using AArch32 (EL0 32-bit)
    .balign 128
    b .

    // 0x680: IRQ from lower EL using AArch32
    .balign 128
    b .

    // 0x700: FIQ from lower EL using AArch32
    .balign 128
    b .

    // 0x780: SError from lower EL using AArch32
    .balign 128
    b .

    // Register the object size for debugging purposes
    .size __vectors_el1, . - __vectors_el1
