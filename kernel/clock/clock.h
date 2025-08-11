// File: kernel/clock/clock.h
// Purpose: Clocksource and clockevent.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_CLOCK_CLOCK_H
#define KERNEL_CLOCK_CLOCK_H

// Initialize the ticker and arm the first tick.
//
// The tick will emit an interrupt.
//
// Requires the trap subsystem to be ready.
void clock_tick_start(void);

// Re-arm the clock for emitting the next interrupt.
//
// Only meaningful after the first tick.
void clock_tick_rearm(void);

#endif // KERNEL_CLOCK_CLOCK_H
