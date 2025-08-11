// File: kernel/sched/clock_arm64.c
// Purpose: ARM64 system clock management
// SPDX-License-Identifier: MIT

#include <kernel/clock/clock.h> // for clock_tick_start
#include <kernel/sched/sched.h> // subsystem API

// Flag indicating we should reschedule
static uint64_t need_sched = 0;

// Number of ticks since the system has booted.
static volatile uint64_t jiffies = 0;

void sched_clock_init_irq(void) {
	clock_tick_start();
}

void sched_clock_irq(void) {
	__atomic_fetch_add(&jiffies, 1, __ATOMIC_RELEASE);
	sched_thread_resume_all(SCHED_THREAD_WAIT_TIMER);
	clock_tick_rearm();
	__atomic_store_n(&need_sched, 1, __ATOMIC_RELEASE);
}

bool __sched_should_reschedule(void) {
	return __atomic_exchange_n(&need_sched, 0, __ATOMIC_ACQUIRE) != 0;
}

uint64_t sched_jiffies(int memoryorder) {
	return __atomic_load_n(&jiffies, memoryorder);
}
