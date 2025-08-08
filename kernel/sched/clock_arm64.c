// File: kernel/sched/clock_arm64.c
// Purpose: ARM64 implementation of clock.h
// SPDX-License-Identifier: MIT

#include <kernel/asm/arm64.h>	// for dsb_sy, etc.
#include <kernel/sched/clock.h> // for sched_clock_init, etc.

// Returns the number of ticks per second used by the hardware.
static inline uint64_t mrs_cntfrq_el0(void) {
	uint64_t v;
	__asm__ volatile("mrs %0, cntfrq_el0" : "=r"(v));
	return v;
}

// Program the clock to fire after the given amount of time has elapsed.
static inline void msr_cntp_tval_el0(uint64_t v) {
	__asm__ volatile("msr cntp_tval_el0, %0" ::"r"(v));
}

// Enable the clock and unmask its interrupt when v is equal to 1.
static inline void msr_cntp_ctl_el0(uint64_t v) {
	__asm__ volatile("msr cntp_ctl_el0, %0" ::"r"(v));
}

// Common function invoked both by sched_clock_init and sched_clock_irq
static void __sched_clock_rearm() {
	// Get the number of ticks per second used by the hardware.
	uint64_t freq = mrs_cntfrq_el0();

	// Scale the number to obtain a frequency of 50 Hz.
	uint64_t ticks_per_int = freq / 50;

	// Program first expiry relative to now
	msr_cntp_tval_el0(ticks_per_int);

	// Enable timer and unmask its interrupt (bit 0 = enable, bit 1 = IMASK)
	msr_cntp_ctl_el0(1);

	// Ensure data consistency and flush instruction buffer.
	dsb_sy();
	isb();
}

void __sched_clock_init(void) {
	__sched_clock_rearm();
}

void sched_clock_irq(void) {
	__sched_clock_rearm();
	// TODO(bassosimone): call into scheduler
	// __sched_preempt_tick();
}
