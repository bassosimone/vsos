// File: kernel/sched/clock.h
// Purpose: use the clock to periodicaly reschedule.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_SCHED_CLOCK_H
#define KERNEL_SCHED_CLOCK_H

#include <kernel/sys/types.h> // for uint64_t

// Initialize the system Timer to tick at 50 Hz and enable its interrupt.
//
// Called by the idle thread when it is starting. Do not use this
// function outside of sched. It is an implementation detail.
void __sched_clock_init(void);

// Interrupt handler for the scheduler clock.
//
// Called from the IRQ vector; must acknowledge and re-arm the timer.
void sched_clock_irq(void);

// Returns true if the current quantum has expired.
bool sched_should_reschedule(void);

// Access the jiffies with the required memory barrier.
//
// Use __ATOMIC_RELAXED unless in IRQ context.
uint64_t sched_jiffies(int memoryorder);

#endif // KERNEL_SCHED_CLOCK_H
