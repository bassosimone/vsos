// File: kernel/clock/clock_arm64.c
// Purpose: ARM64 clocksource and clockevent.
// SPDX-License-Identifier: MIT

#include <kernel/asm/arm64.h>	// for mrs_cntfrq_el0
#include <kernel/clock/clock.h> // for clock_init_early
#include <kernel/core/printk.h> // for printk

#include <sys/param.h> // for HZ

static uint64_t ticks_per_interval = 0;

void clock_tick_start(void) {
	// Get the number of ticks per second used by the hardware.
	uint64_t freq = mrs_cntfrq_el0();

	// Scale the number to obtain a frequency of HZ Hertz.
	ticks_per_interval = freq / HZ;

	// Program first expiry relative to now
	msr_cntp_tval_el0(ticks_per_interval);

	// Enable timer and unmask its interrupt (bit 0 = enable, bit 1 = IMASK)
	msr_cntp_ctl_el0(1);

	// Ensure the new control/tval are visible to the core before continuing.
	isb();

	// Let the user know we programmed the ticker.
	printk("clock0: ticking %lld times per second\n", HZ);
}

void clock_tick_rearm(void) {
	msr_cntp_tval_el0(ticks_per_interval);
	isb();
}
