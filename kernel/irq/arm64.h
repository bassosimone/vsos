// File: kernel/irq/arm64.h
// Purpose: IRQ management in ARM64
// SPDX-License-Identifier: MIT
#ifndef KERNEL_IRQ_ARM64_H
#define KERNEL_IRQ_ARM64_H

#include <kernel/sys/types.h>

// Structure saving the pre-interrupt state.
struct trapframe {
	uint64_t x[31];
	uint64_t sp_el0;
	__uint128_t q[32];
	uint64_t elr_el1;
	uint64_t spsr_el1;
	uint64_t fpcr;
	uint64_t fpsr;
} __attribute__((aligned(16)));

// Make sure the C struct is synchronized with the assembly code
static_assert(alignof(struct trapframe) == 16, "trapframe must be 16B aligned");
static_assert(sizeof(struct trapframe) == 800, "trapframe must be 800 bytes");
static_assert(__builtin_offsetof(struct trapframe, x) == 0, "x offset");
static_assert(__builtin_offsetof(struct trapframe, sp_el0) == 248, "sp_el0 offset");
static_assert(__builtin_offsetof(struct trapframe, q) == 256, "q block offset");
static_assert(__builtin_offsetof(struct trapframe, elr_el1) == 768, "elr_el1 offset");
static_assert(__builtin_offsetof(struct trapframe, spsr_el1) == 776, "spsr_el1 offset");
static_assert(__builtin_offsetof(struct trapframe, fpcr) == 784, "fpcr offset");
static_assert(__builtin_offsetof(struct trapframe, fpsr) == 792, "fpsr offset");

#endif // KERNEL_IRQ_ARM64_H
