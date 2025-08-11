    // File: kernel/sched/switch_arm64.S
    // Purpose: ARM64 kernel thread switching code
    // SPDX-License-Identifier: MIT

    .section .text
    .global __sched_switch
    .type __sched_switch, %function

    // void __sched_switch(struct sched_thread *x0, struct sched_thread *x1);
    //
    // This function assumes to be called with interrupts disabled.
__sched_switch:
    // Step 0: skip saving if x0 is zero
    cbz x0, 1f

    // Step 1: save current thread state to prev->sp.
    //
    // 1.1. reserve the required stack space
    sub sp, sp, #240  // Note: keep in sync with __sched_build_switch_frame

    // 1.2. save GPRs (x19–x30) individually, 8 bytes each
    stp x19, x20, [sp, #0]
    stp x21, x22, [sp, #16]
    stp x23, x24, [sp, #32]
    stp x25, x26, [sp, #48]
    stp x27, x28, [sp, #64]
    stp x29, x30, [sp, #80] // Note: keep in sync with __sched_build_switch_frame

    // 1.3. save SIMD registers (q8–q15), 16 bytes each
    stp q8, q9, [sp, #96]
    stp q10, q11, [sp, #128]
    stp q12, q13, [sp, #160]
    stp q14, q15, [sp, #192]

    // 1.4. save floating point control/status registers
    //
    // They are 4 bytes each but we save 8 bytes for alignment
    mrs x19, fpcr
    mrs x20, fpsr
    stp x19, x20, [sp, #224]

    // 1.5. save updated SP into prev->sp.
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
    // 2. next->sp is the first entry in next
1:
    ldr x2, [x1]
    mov sp, x2

    // 2.2. restore floating point control/status registers
    ldp x19, x20, [sp, #224]
    msr fpcr, x19
    msr fpsr, x20

    // 2.3. Restore SIMD registers (q8–q15)
    ldp q14, q15, [sp, #192]
    ldp q12, q13, [sp, #160]
    ldp q10, q11, [sp, #128]
    ldp q8, q9, [sp, #96]

    // 2.4. restore GPRs (x19-x30)
    ldp x29, x30, [sp, #80]
    ldp x27, x28, [sp, #64]
    ldp x25, x26, [sp, #48]
    ldp x23, x24, [sp, #32]
    ldp x21, x22, [sp, #16]
    ldp x19, x20, [sp, #0]

    // 2.5. unwind the stack
    add sp, sp, #240

    // 3. return to restored x30
    ret

    // uintptr_t __sched_build_switch_frame(uintptr_t sp);
    //
    // Builds the switch frame pointing to the trampoline
    // and returns the modified stack pointer.
    //
    // Assumes that the stack has been zeroed before
    // invoking this function.
    .global __sched_build_switch_frame
    .type   __sched_build_switch_frame, %function
    .extern __sched_trampoline

__sched_build_switch_frame:
    // Reserve switch frame
    sub x0, x0, #240  // Note: keep in sync with __sched_switch

    // Set the trampoline correctly
    mov x9, #0
    adrp x10, __sched_trampoline
    add x10, x10, :lo12:__sched_trampoline
    stp x9, x10, [x0, #80]  // Note: keep in sync with __sched_switch

    // Our job here is done
    ret
