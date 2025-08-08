// File: kernel/sched/clock.h
// Purpose: use the clock to periodicaly reschedule.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_SCHED_CLOCK_H
#define KERNEL_SCHED_CLOCK_H

// Initialize the system Timer to tick at 50 Hz and enable its interrupt.
//
// Called by the idle thread when it is starting. Do not use this
// function outside of sched. It is an implementation detail.
void __sched_clock_init(void);

// Interrupt handler for the scheduler clock.
//
// Called from the IRQ vector; must acknowledge and re-arm the timer.
void sched_clock_irq(void);

#endif // KERNEL_SCHED_CLOCK_H
