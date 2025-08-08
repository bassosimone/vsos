// File: kernel/sched/thread_arm64.c
// Purpose: ARM64-specific thread code
// SPDX-License-Identifier: MIT

#include <kernel/asm/arm64.h>	 // for wfi
#include <kernel/sched/thread.h> // for sched_thread, etc.
#include <libc/string/string.h>	 // for memset

void __sched_thread_stack_init(struct sched_thread *thread) {
	// See the stack as a uint8 aligned array
	uintptr_t *sp = (uintptr_t *)&thread->stack[SCHED_THREAD_STACK_SIZE];

	// Mirror the first half of __sched_switch in switch_arm64.S
	//
	// Step 1: Save GPRs (x19–x30) individually
	sp--;
	*sp = 0; // x19

	sp--;
	*sp = 0; // x20

	sp--;
	*sp = 0; // x21

	sp--;
	*sp = 0; // x22

	sp--;
	*sp = 0; // x23

	sp--;
	*sp = 0; // x24

	sp--;
	*sp = 0; // x25

	sp--;
	*sp = 0; // x26

	sp--;
	*sp = 0; // x27

	sp--;
	*sp = 0; // x28

	sp--;
	*sp = 0; // x29

	// x30 (LR)
	sp--;
	*sp = (uintptr_t)__sched_trampoline;

	// Step 2: Reserve SIMD (128 bytes)
	sp = (uintptr_t *)((uintptr_t)sp - 128);
	memset(sp, 0, 128);

	// Step 3: Align SP (optional but good)
	sp = (uintptr_t *)((uintptr_t)sp & ~0xF);

	thread->sp = (uintptr_t)sp;
}

void __sched_idle(void) {
	wfi();
}

void sched_thread_yield(void) {
	// Disable interrupts while switching
	msr_daifset_2();

	// Perform the actual switch
	__sched_thread_yield_without_interrupts();

	// Re-enable interrupts when done
	msr_daifclr_2();
}
