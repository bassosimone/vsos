// File: kernel/trap/trap_arm64.h
// Purpose: Trap management in ARM64
// SPDX-License-Identifier: MIT
#ifndef KERNEL_TRAP_TRAP_ARM64_H
#define KERNEL_TRAP_TRAP_ARM64_H

#include <sys/types.h>

// Structure saving the pre-trap state.
struct trap_frame {
	uint64_t x[31];
	uint64_t sp_el0;
	__uint128_t q[32];
	uint64_t elr_el1;
	uint64_t spsr_el1;
	uint64_t fpcr;
	uint64_t fpsr;
	uint64_t ttbr0_el1;
	uint64_t __unused_padding;
} __attribute__((aligned(16)));

// Make sure the C struct is synchronized with the assembly code
static_assert(alignof(struct trap_frame) == 16, "trap_frame must be 16B aligned");
static_assert(sizeof(struct trap_frame) == 816, "trap_frame must be 816 bytes");
static_assert(__builtin_offsetof(struct trap_frame, x) == 0, "x offset");
static_assert(__builtin_offsetof(struct trap_frame, sp_el0) == 248, "sp_el0 offset");
static_assert(__builtin_offsetof(struct trap_frame, q) == 256, "q block offset");
static_assert(__builtin_offsetof(struct trap_frame, elr_el1) == 768, "elr_el1 offset");
static_assert(__builtin_offsetof(struct trap_frame, spsr_el1) == 776, "spsr_el1 offset");
static_assert(__builtin_offsetof(struct trap_frame, fpcr) == 784, "fpcr offset");
static_assert(__builtin_offsetof(struct trap_frame, fpsr) == 792, "fpsr offset");
static_assert(__builtin_offsetof(struct trap_frame, ttbr0_el1) == 800, "ttbr0_el1 offset");
static_assert(__builtin_offsetof(struct trap_frame, __unused_padding) == 808, "__unused_padding offset");

// Generic interrupt service routine.
//
// Called by the trap handlers written in assembly.
void __trap_isr(struct trap_frame *frame);

// Generic synchronous service routine.
//
// Called by the trap handlers written in assembly.
void __trap_ssr(struct trap_frame *frame, uint64_t esr, uint64_t far);

#endif // KERNEL_TRAP_TRAP_ARM64_H
