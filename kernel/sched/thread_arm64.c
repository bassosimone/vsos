// File: kernel/sched/thread_arm64.c
// Purpose: ARM64-specific thread code
// SPDX-License-Identifier: MIT

#include <kernel/asm/asm.h>	// for local_irq_disable
#include <kernel/sched/sched.h> // subsystem API

void __sched_idle(void) {
	cpu_sleep_until_interrupt();
}

void sched_thread_yield(void) {
	// Disable interrupts while switching
	local_irq_disable();

	// Perform the actual switch
	__sched_thread_yield();

	// Re-enable interrupts when done
	local_irq_enable();
}
