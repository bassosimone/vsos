    // File: kernel/trap/handle_arm64.S
    // Purpose: ARM64 trap handlers implementation
    // SPDX-License-Identifier: MIT

    // void __trap_handle_el1h_irq(void);
    //
    // Handles interrupts when running at kernel level.
    .section .text
    .global __trap_handle_el1h_irq
    .align 4
    .type __trap_handle_el1h_irq, %function
    .extern __trap_isr
__trap_handle_el1h_irq:
    // Interrupts are disabled when we enter here and we keep
    // them disabled for the whole handler duration.
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
    mov x0, sp
    bl  __trap_isr

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

    // void __trap_handle_synchronous(void);
    //
    // Handles syscalls.
    .section .text
    .global __trap_handle_synchronous
    .align 4
    .type __trap_handle_synchronous, %function
    .extern __syscall_handle
    .extern sched_return_to_user
__trap_handle_synchronous:
    // Interrupts are disabled when we enter here and we keep
    // them disabled for the whole handler duration.
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

    // Handle the syscall passing the frame, esr, and far
    mov x0, sp
    mrs x1, esr_el1
    mrs x2, far_el1
    bl  __trap_ssr

    // Tail call the code to return to user space including
    // possibly performing a context switch
    mov x0, sp
    b sched_return_to_user

    // void trap_restore_user_and_eret(uintptr_t trapframe);
    //
    // Handles returning from userspace from traps.
    .section .text
    .global trap_restore_user_and_eret
    .align 4
    .type trap_restore_user_and_eret, %function
trap_restore_user_and_eret:
    // Adjust the kernel stack back
    add sp, x0, #800

    // Restore floating point control/status registers
    ldr x20, [x0, #792]
    msr fpsr, x20
    ldr x19, [x0, #784]
    msr fpcr, x19

    // Restore control registers
    ldr x20, [x0, #776]
    msr spsr_el1, x20
    ldr x19, [x0, #768]
    msr elr_el1, x19

    // Restore all SIMD/FP registers (q0-q31)
    ldp q30, q31, [x0, #736]
    ldp q28, q29, [x0, #704]
    ldp q26, q27, [x0, #672]
    ldp q24, q25, [x0, #640]
    ldp q22, q23, [x0, #608]
    ldp q20, q21, [x0, #576]
    ldp q18, q19, [x0, #544]
    ldp q16, q17, [x0, #512]
    ldp q14, q15, [x0, #480]
    ldp q12, q13, [x0, #448]
    ldp q10, q11, [x0, #416]
    ldp q8,  q9,  [x0, #384]
    ldp q6,  q7,  [x0, #352]
    ldp q4,  q5,  [x0, #320]
    ldp q2,  q3,  [x0, #288]
    ldp q0,  q1,  [x0, #256]

    // Restore general purpose registers and user stack pointer
    ldp x30, x19, [x0, #240]
    msr sp_el0, x19
    ldp x28, x29, [x0, #224]
    ldp x26, x27, [x0, #208]
    ldp x24, x25, [x0, #192]
    ldp x22, x23, [x0, #176]
    ldp x20, x21, [x0, #160]
    ldp x18, x19, [x0, #144]
    ldp x16, x17, [x0, #128]
    ldp x14, x15, [x0, #112]
    ldp x12, x13, [x0, #96]
    ldp x10, x11, [x0, #80]
    ldp x8,  x9,  [x0, #64]
    ldp x6,  x7,  [x0, #48]
    ldp x4,  x5,  [x0, #32]
    ldp x2,  x3,  [x0, #16]

    // Be extra careful to avoid clobbering x0
    ldr x1, [x0, #8]
    ldr x0, [x0, #0]

    // Return from exception
    eret
