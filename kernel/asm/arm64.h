// File: kernel/asm/arm64.h
// Purpose: ARM64 assembly routines
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#ifndef KERNEL_ASM_ARM64_H
#define KERNEL_ASM_ARM64_H

#include <kernel/sys/types.h>

// WFI: wait for interrupts
static inline void wfi(void) { __asm__ volatile("wfi"); }

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
static inline void dsb_sy(void) { __asm__ volatile("dsb sy" ::: "memory"); }

// ISB: instruction synchronization barrier.
//
// Required after system register writes (e.g., TTBR).
static inline void isb(void) { __asm__ volatile("isb" ::: "memory"); }

// DMB: data memory barrier.
//
// Before a read, similar to Zig's `.Acquire`.
//
// After a write, similar to Zig's `.Release`.
static inline void dmb_sy(void) { __asm__ volatile("dmb sy" ::: "memory"); }

#endif // KERNEL_ASM_ARM64
