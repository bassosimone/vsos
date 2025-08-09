// File: kernel/sched/thread_arm64.c
// Purpose: ARM64-specific thread code
// SPDX-License-Identifier: MIT

#include <kernel/asm/arm64.h>	// for wfi
#include <kernel/sched/sched.h> // subsystem API

void __sched_idle(void) {
	wfi();
}

void sched_thread_yield(void) {
	// Disable interrupts while switching
	msr_daifset_2();

	// Perform the actual switch
	__sched_thread_yield();

	// Re-enable interrupts when done
	msr_daifclr_2();
}
