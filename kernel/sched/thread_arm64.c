// File: kernel/sched/thread_arm64.c
// Purpose: ARM64-specific thread code
// SPDX-License-Identifier: MIT

#include "kernel/core/assert.h"
#include <kernel/asm/arm64.h>	 // for wfi
#include <kernel/sched/thread.h> // for sched_thread, etc.
#include <libc/string/string.h>	 // for memset

// Implemented in assembly
uintptr_t __sched_build_switch_frame(uintptr_t sp);

void __sched_thread_stack_init(struct sched_thread *thread) {
	// See the stack as a uint8 aligned array
	uintptr_t sp = (uintptr_t)&thread->stack[SCHED_THREAD_STACK_SIZE];

	// Ensure the stack is 16 byte aligned
	KERNEL_ASSERT((sp & 0xF) == 0);

	// Use assembly magic to create the switch frame
	thread->sp = __sched_build_switch_frame(sp);
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
