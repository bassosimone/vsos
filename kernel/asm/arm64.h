// File: kernel/asm/arm64.h
// Purpose: ARM64 assembly routines
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#ifndef KERNEL_ASM_ARM64_H
#define KERNEL_ASM_ARM64_H

#include <kernel/sys/types.h>

// DSB: data synchronization barrier.
//
// Similar to Zig's `.SeqCst`.
//
// Use this for:
//
// 1. MMIO device register writes
//
// 2. Page table updates
//
// 3. System state transitions
static inline void dsb_sy(void) {
	__asm__ volatile("dsb sy" ::: "memory");
}

// WFI: wait for interrupts
static inline void wfi(void) {
	dsb_sy();
	__asm__ volatile("wfi");
}

// ISB: instruction synchronization barrier.
//
// Required after system register writes (e.g., TTBR, CPACR).
static inline void isb(void) {
	__asm__ volatile("isb" ::: "memory");
}

// Enable or disable access to FP/SIMD registers at EL0 and EL1.
//
// The CPACR_EL1 register controls this access via bits 20:21 (FPEN):
//
// - 0b00: Trap FP/SIMD at both EL0 and EL1
// - 0b01: Trap at EL0 only
// - 0b11: Allow FP/SIMD at EL0 and EL1
//
// Note: despite the _EL1 suffix, this register controls *both* EL0 and EL1
// behavior, from the perspective of EL1.
static inline void __enable_disable_fp_simd(bool enable) {
	// Read the register
	uint64_t cpacr;
	__asm__ volatile("mrs %0, cpacr_el1" : "=r"(cpacr));

	// Either set FPEN to 0b11 or clear it
	if (enable) {
		cpacr |= (3 << 20);
	} else {
		cpacr &= ~(3 << 20);
	}

	// Write the bits back
	__asm__ volatile("msr cpacr_el1, %0" ::"r"(cpacr));

	// Ensure the change takes effect immediately
	isb();
}

// Enable FP/SIMD for both kernel (EL1) and user space (EL0).
//
// Required if Clang generates NEON or floating-point instructions (e.g. for
// `printk`, `memcpy`, etc.). Without this, such instructions trap at 0x200 with
// exception class 0x7 (Undefined Instruction).
static inline void enable_fp_simd(void) {
	__enable_disable_fp_simd(true);
}

// Disallow FP/SIMD usage at EL0 and EL1 and restores traps on use.
static inline void disable_fp_simd(void) {
	__enable_disable_fp_simd(false);
}

// DMB: data memory barrier.
//
// Before a read, similar to Zig's `.Acquire`.
//
// After a write, similar to Zig's `.Release`.
static inline void dmb_sy(void) {
	__asm__ volatile("dmb sy" ::: "memory");
}

#endif // KERNEL_ASM_ARM64
