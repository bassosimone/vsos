    // File: kernel/boot/irq_vector_arm64.S
    // Purpose: interrupt vector for arm64
    // SPDX-License-Identifier: MIT

    .section .vectors, "ax", %progbits
    .align 11
    .global __vectors_el1
    .type   __vectors_el1, %function
__vectors_el1:
    // 0x000: Synchronous EL1t
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

    // 0x200: Synchronous EL1h
    .balign 128
    b .

    // 0x280: IRQ EL1h
    .balign 128
    b el1h_irq

    // 0x300: FIQ EL1h
    .balign 128
    b .

    // 0x380: SError EL1h
    .balign 128
    b .

    .section .text
    .global el1h_irq
    .align 4
    .type el1h_irq, %function
el1h_irq:
    // Interrupts are disabled when we enter here and we keep
    // them disable for the whole handler duration.
    //
    // Basically this means no nested interrupts.

    // Bump the stack frame to make space for all the variables to save.
    sub sp, sp, #800

    // Save general purpose registers and user stack pointer.
    stp x0,  x1,  [sp, #0]
    stp x2,  x3,  [sp, #16]
    stp x4,  x5,  [sp, #32]
    stp x6,  x7,  [sp, #48]
    stp x8,  x9,  [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    mrs x19, sp_el0
    stp x30, x19, [sp, #240]

    // Save all SIMD/FP registers (q0-q31)
    stp q0, q1, [sp, #256]
    stp q2, q3, [sp, #288]
    stp q4, q5, [sp, #320]
    stp q6, q7, [sp, #352]
    stp q8, q9, [sp, #384]
    stp q10, q11, [sp, #416]
    stp q12, q13, [sp, #448]
    stp q14, q15, [sp, #480]
    stp q16, q17, [sp, #512]
    stp q18, q19, [sp, #544]
    stp q20, q21, [sp, #576]
    stp q22, q23, [sp, #608]
    stp q24, q25, [sp, #640]
    stp q26, q27, [sp, #672]
    stp q28, q29, [sp, #704]
    stp q30, q31, [sp, #736]

    // Save control registers
    mrs x19, elr_el1
    str x19, [sp, #768]
    mrs x20, spsr_el1
    str x20, [sp, #776]

    // Save floating point control/status registers
    mrs x19, fpcr
    str x19, [sp, #784]
    mrs x20, fpsr
    str x20, [sp, #792]

    // Handle the interrupt passing the frame as context
    mov     x0, sp
    bl      irq_dispatch_el1h

    // Restore floating point control/status registers
    ldr x20, [sp, #792]
    msr fpsr, x20
    ldr x19, [sp, #784]
    msr fpcr, x19

    // Restore control registers
    ldr x20, [sp, #776]
    msr spsr_el1, x20
    ldr x19, [sp, #768]
    msr elr_el1, x19

    // Restore all SIMD/FP registers (q0-q31)
    ldp q30, q31, [sp, #736]
    ldp q28, q29, [sp, #704]
    ldp q26, q27, [sp, #672]
    ldp q24, q25, [sp, #640]
    ldp q22, q23, [sp, #608]
    ldp q20, q21, [sp, #576]
    ldp q18, q19, [sp, #544]
    ldp q16, q17, [sp, #512]
    ldp q14, q15, [sp, #480]
    ldp q12, q13, [sp, #448]
    ldp q10, q11, [sp, #416]
    ldp q8,  q9,  [sp, #384]
    ldp q6,  q7,  [sp, #352]
    ldp q4,  q5,  [sp, #320]
    ldp q2,  q3,  [sp, #288]
    ldp q0,  q1,  [sp, #256]

    // Restore general purpose registers and user stack pointer
    ldp x30, x19, [sp, #240]
    msr sp_el0, x19
    ldp x28, x29, [sp, #224]
    ldp x26, x27, [sp, #208]
    ldp x24, x25, [sp, #192]
    ldp x22, x23, [sp, #176]
    ldp x20, x21, [sp, #160]
    ldp x18, x19, [sp, #144]
    ldp x16, x17, [sp, #128]
    ldp x14, x15, [sp, #112]
    ldp x12, x13, [sp, #96]
    ldp x10, x11, [sp, #80]
    ldp x8,  x9,  [sp, #64]
    ldp x6,  x7,  [sp, #48]
    ldp x4,  x5,  [sp, #32]
    ldp x2,  x3,  [sp, #16]
    ldp x0,  x1,  [sp, #0]

    // Unwind the stack
    add sp, sp, #800

    // Return from exception
    eret
