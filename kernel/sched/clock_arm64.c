// File: kernel/sched/clock_arm64.c
// Purpose: ARM64 implementation of clock.h
// SPDX-License-Identifier: MIT

#include <kernel/asm/arm64.h>	 // for dsb_sy, etc.
#include <kernel/sched/clock.h>	 // for sched_clock_init, etc.
#include <kernel/sched/thread.h> // for sched_thread_resume_all
#include <kernel/sys/param.h>	 // for HZ

// Flag indicating we should reschedule
static uint64_t need_sched = 0;

// Number of ticks since the system has booted.
static volatile uint64_t jiffies = 0;

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

	// Scale the number to obtain a frequency of HZ Hertz.
	uint64_t ticks_per_int = freq / HZ;

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
	__atomic_fetch_add(&jiffies, 1, __ATOMIC_RELEASE);
	sched_thread_resume_all(SCHED_THREAD_WAIT_TIMER);
	__sched_clock_rearm();
	__atomic_store_n(&need_sched, 1, __ATOMIC_RELEASE);
}

bool sched_should_reschedule(void) {
	return __atomic_exchange_n(&need_sched, 0, __ATOMIC_ACQUIRE) != 0;
}

uint64_t sched_jiffies(int memoryorder) {
	return __atomic_load_n(&jiffies, memoryorder);
}
