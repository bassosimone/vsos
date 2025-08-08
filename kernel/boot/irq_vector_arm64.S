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
    // Frame size = 288 bytes (16-byte aligned)
    // Layout:
    //   [  0] q0  [ 16] q1  [ 32] q2  [ 48] q3  [ 64] q4  [ 80] q5  [ 96] q6 [112] q7
    //
    //   [128] x0  [136] x1  [144] x2  [152] x3  [160] x4  [168] x5  [176] x6
    //   [184] x7  [192] x8  [200] x9  [208] x10 [216] x11 [224] x12 [232] x13
    //   [240] x14 [248] x15 [256] x16 [264] x17 [272] x18 [280] x30
    sub     sp, sp, #288

    // Save SIMD/FP q0–q7 (caller-saved)
    str     q0,  [sp, #0]
    str     q1,  [sp, #16]
    str     q2,  [sp, #32]
    str     q3,  [sp, #48]
    str     q4,  [sp, #64]
    str     q5,  [sp, #80]
    str     q6,  [sp, #96]
    str     q7,  [sp, #112]

    // Save GPR caller-saved x0–x18 and LR (x30)
    str     x0,  [sp, #128]
    str     x1,  [sp, #136]
    str     x2,  [sp, #144]
    str     x3,  [sp, #152]
    str     x4,  [sp, #160]
    str     x5,  [sp, #168]
    str     x6,  [sp, #176]
    str     x7,  [sp, #184]
    str     x8,  [sp, #192]
    str     x9,  [sp, #200]
    str     x10, [sp, #208]
    str     x11, [sp, #216]
    str     x12, [sp, #224]
    str     x13, [sp, #232]
    str     x14, [sp, #240]
    str     x15, [sp, #248]
    str     x16, [sp, #256]
    str     x17, [sp, #264]
    str     x18, [sp, #272]
    str     x30, [sp, #280]

    bl      irq_dispatch_el1h

    // Restore GPRs
    ldr     x30, [sp, #280]
    ldr     x18, [sp, #272]
    ldr     x17, [sp, #264]
    ldr     x16, [sp, #256]
    ldr     x15, [sp, #248]
    ldr     x14, [sp, #240]
    ldr     x13, [sp, #232]
    ldr     x12, [sp, #224]
    ldr     x11, [sp, #216]
    ldr     x10, [sp, #208]
    ldr     x9,  [sp, #200]
    ldr     x8,  [sp, #192]
    ldr     x7,  [sp, #184]
    ldr     x6,  [sp, #176]
    ldr     x5,  [sp, #168]
    ldr     x4,  [sp, #160]
    ldr     x3,  [sp, #152]
    ldr     x2,  [sp, #144]
    ldr     x1,  [sp, #136]
    ldr     x0,  [sp, #128]

    // Restore SIMD/FP
    ldr     q7,  [sp, #112]
    ldr     q6,  [sp, #96]
    ldr     q5,  [sp, #80]
    ldr     q4,  [sp, #64]
    ldr     q3,  [sp, #48]
    ldr     q2,  [sp, #32]
    ldr     q1,  [sp, #16]
    ldr     q0,  [sp, #0]

    add     sp, sp, #288
    eret
