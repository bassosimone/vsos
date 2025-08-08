    // File: kernel/sched/switch_arm64.S
    // Purpose: ARM64 process switching code
    // SPDX-License-Identifier: MIT

    .section .text
    .global __sched_switch
    .type __sched_switch, %function

    // void __sched_switch(struct sched_thread *x0, struct sched_thread *x1)
__sched_switch:
    // TODO(bassosimone): when we start using the timer to
    // switch process, we'll additionally need to disable
    // interrupts while switching is in progress otherwise
    // we could clobber the switching state.

    // Step 0: skip saving if x0 is zero
    cbz x0, 1f

    // Step 1: save current thread state to prev->sp.
    //
    // 1.1. save GPRs (x19–x30) individually.
    sub sp, sp, #8
    str x19, [sp]

    sub sp, sp, #8
    str x20, [sp]

    sub sp, sp, #8
    str x21, [sp]

    sub sp, sp, #8
    str x22, [sp]

    sub sp, sp, #8
    str x23, [sp]

    sub sp, sp, #8
    str x24, [sp]

    sub sp, sp, #8
    str x25, [sp]

    sub sp, sp, #8
    str x26, [sp]

    sub sp, sp, #8
    str x27, [sp]

    sub sp, sp, #8
    str x28, [sp]

    sub sp, sp, #8
    str x29, [sp]

    sub sp, sp, #8
    str x30, [sp]

    // 1.2. save SIMD registers (q8–q15), 16 bytes each
    sub sp, sp, #16
    str q8,  [sp]

    sub sp, sp, #16
    str q9,  [sp]

    sub sp, sp, #16
    str q10, [sp]

    sub sp, sp, #16
    str q11, [sp]

    sub sp, sp, #16
    str q12, [sp]

    sub sp, sp, #16
    str q13, [sp]

    sub sp, sp, #16
    str q14, [sp]

    sub sp, sp, #16
    str q15, [sp]

    // 1.3. save updated SP into prev->sp.
    //
    // Assumptions:
    //
    // 1. the prev value is in x0
    //
    // 2. prev->sp is the first entry in prev
    mov x2, sp
    str x2, [x0]

    // Step 2: restore the new thread state.
    //
    // 2.1. Load SP from next->sp
    //
    // Assumptions:
    //
    // 1. the next value is in x1
    //
    // 2. next->sp is the first entry in prev
1:
    ldr x2, [x1]
    mov sp, x2

    // 2.2. Restore SIMD q8–q15
    ldr q15, [sp]
    add sp, sp, #16

    ldr q14, [sp]
    add sp, sp, #16

    ldr q13, [sp]
    add sp, sp, #16

    ldr q12, [sp]
    add sp, sp, #16

    ldr q11, [sp]
    add sp, sp, #16

    ldr q10, [sp]
    add sp, sp, #16

    ldr q9,  [sp]
    add sp, sp, #16

    ldr q8,  [sp]
    add sp, sp, #16

    // 2.3. Restore GPRs (x30 down to x19)
    ldr x30, [sp]
    add sp, sp, #8

    ldr x29, [sp]
    add sp, sp, #8

    ldr x28, [sp]
    add sp, sp, #8

    ldr x27, [sp]
    add sp, sp, #8

    ldr x26, [sp]
    add sp, sp, #8

    ldr x25, [sp]
    add sp, sp, #8

    ldr x24, [sp]
    add sp, sp, #8

    ldr x23, [sp]
    add sp, sp, #8

    ldr x22, [sp]
    add sp, sp, #8

    ldr x21, [sp]
    add sp, sp, #8

    ldr x20, [sp]
    add sp, sp, #8

    ldr x19, [sp]
    add sp, sp, #8

    // 3. Return to restored x30
    ret
