    // File: kernel/boot/boot_arm64.S
    // Purpose: ARM64 assembly boot entry point
    // SPDX-License-Identifier: MIT

    .section ".text.boot"
    .global boot

boot:
    // 1. Enable FP/SIMD
    mrs x9, cpacr_el1         // Use x9 as temp register
    orr x9, x9, #(3 << 20)    // Set FPEN bits to 0b11
    msr cpacr_el1, x9
    isb                       // Synchronize instruction stream
    
    // 2. Set stack pointer right before jumping
    ldr x9, =__stack_top      // Load stack address into temp register
    mov sp, x9                // Set stack pointer
    
    // 3. Jump to MI kernel code
    bl __kernel_main
