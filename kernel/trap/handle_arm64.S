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
    // Frame layout documented in kernel/trap/trap_arm64.h (struct trap_frame)
    sub sp, sp, #816

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

    // Save the TTBR0_EL1 and the padding
    mrs x9, ttbr0_el1
    mov x10, #0
    str x9, [sp, #800]
    str x10, [sp, #808]

    // Switch page tables and use the kernel one
    ldr x11, =__vm_kernel_root_pt
    ldr x11, [x11]
    msr ttbr0_el1, x11
    isb

    // Handle the interrupt passing the frame as context
    mov x0, sp
    bl  __trap_isr

    // Restore TTBR0_EL1 and the padding
    ldr x10, [sp, #808]
    ldr x9, [sp, #800]
    msr ttbr0_el1, x9
    isb

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
    add sp, sp, #816

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
    // Frame layout documented in kernel/trap/trap_arm64.h (struct trap_frame)
    sub sp, sp, #816

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

    // Save the TTBR0_EL1 and the padding
    mrs x9, ttbr0_el1
    mov x10, #0
    str x9, [sp, #800]
    str x10, [sp, #808]

    // Switch page tables and use the kernel one
    ldr x11, =__vm_kernel_root_pt
    ldr x11, [x11]
    msr ttbr0_el1, x11
    isb

    // Handle the syscall passing the frame, esr, and far
    mov x0, sp
    mrs x1, esr_el1
    mrs x2, far_el1
    bl  __trap_ssr

    // Tail call the code to return to user space including
    // possibly performing a context switch
    mov x0, sp
    b sched_return_to_user

    // void __trap_restore_user_and_eret(uintptr_t trapframe);
    //
    // Handles returning from userspace from traps.
    .section .text
    .global __trap_restore_user_and_eret
    .align 4
    .type __trap_restore_user_and_eret, %function
__trap_restore_user_and_eret:
    // Adjust the kernel stack back
    add sp, x0, #816

    // Restore TTBR0_EL1 and the padding
    ldr x10, [x0, #808]
    ldr x9, [x0, #800]
    msr ttbr0_el1, x9
    isb

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

    // uintptr_t trap_create_process_frame(uintptr_t entry, uintptr_t root_pt, uintpt_t ustack);
    //
    // Creates a process frame allowing return to userspace with ERET.
    .section .text
    .global trap_create_process_frame
    .align 4
    .type trap_create_process_frame, %function
trap_create_process_frame:
    // Arguments: x0 = entry point, x1 = page table, x2 = user stack

    // Bump the stack frame to make space for all the variables to save.
    // Frame layout documented in kernel/trap/trap_arm64.h (struct trap_frame)
    sub sp, sp, #816

    // Move zero to x9 so we can use it for setting things to zero.
    //
    // This seems a register we as the callee can clobber.
    mov x9, #0

    // Save general purpose registers and user stack pointer.
    stp x9, x9, [sp, #0]    // x0, x1
    stp x9, x9, [sp, #16]   // x2, x3
    stp x9, x9, [sp, #32]   // x4, x5
    stp x9, x9, [sp, #48]   // x6, x7
    stp x9, x9, [sp, #64]   // x8, x9
    stp x9, x9, [sp, #80]   // x10, x11
    stp x9, x9, [sp, #96]   // x12, x13
    stp x9, x9, [sp, #112]  // x14, x15
    stp x9, x9, [sp, #128]  // x16, x17
    stp x9, x9, [sp, #144]  // x18, x19
    stp x9, x9, [sp, #160]  // x20, x21
    stp x9, x9, [sp, #176]  // x22, x23
    stp x9, x9, [sp, #192]  // x24, x25
    stp x9, x9, [sp, #208]  // x26, x27
    stp x9, x9, [sp, #224]  // x28, x29
    stp x9, x2, [sp, #240]  // x30, <user_stack>

    // Move zero to q0 so we can use it for settings things to zero
    //
    // This seems a register we as the callee can clobber.
    movi v0.2d, #0

    // Save all SIMD/FP registers (q0-q31)
    stp q0, q0, [sp, #256] // q0, q1
    stp q0, q0, [sp, #288] // q2, q3
    stp q0, q0, [sp, #320] // q4, q5
    stp q0, q0, [sp, #352] // q6, q7
    stp q0, q0, [sp, #384] // q8, q9
    stp q0, q0, [sp, #416] // q10, q11
    stp q0, q0, [sp, #448] // q12, q13
    stp q0, q0, [sp, #480] // q14, q15
    stp q0, q0, [sp, #512] // q16, q17
    stp q0, q0, [sp, #544] // q18, q19
    stp q0, q0, [sp, #576] // q20, q21
    stp q0, q0, [sp, #608] // q22, q23
    stp q0, q0, [sp, #640] // q24, q25
    stp q0, q0, [sp, #672] // q26, q27
    stp q0, q0, [sp, #704] // q28, q29
    stp q0, q0, [sp, #736] // q30, q31

    // Save control registers
    str x0, [sp, #768]   // elr_el1 = <entry>
    mov x10, #0x0        // EL0t mode, interrupts enabled
    str x10, [sp, #776]  // spsr_el1 = userspace state

    // Save floating point control/status registers
    mov x10, #0x0        // sensible FP default
    str x10, [sp, #784]  // fpcr = 0
    str x10, [sp, #792]  // fpsr = 0

    // Save the TTBR0_EL1 and the padding
    str x1, [sp, #800]   // ttbr0_el1
    str x9, [sp, #808]   // padding

    // Return the current stack pointer so to emulate
    // when happens when we take a synch trap.
    mov x0, sp
    ret
