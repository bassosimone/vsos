// File: kernel/sched/switch.h
// Purpose: Kernel-thread switch implementation.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_SCHED_SWITCH_H
#define KERNEL_SCHED_SWITCH_H

#include <sys/types.h> // for uintptr_t

// Forward sched_thread declaration.
struct sched_thread;

// Machine-dependent context-switch implementation.
//
// "You are not expected to understand this".
//
// Called by the scheduler internals.
//
// Do not use outside of this subsystem.
void __sched_switch(struct sched_thread *prev, struct sched_thread *next);

// Trampoline for starting a new kernel thread.
//
// Called by the scheduler internals.
//
// Do not use outside of this subsystem.
void __sched_trampoline(void);

// Setup a kernel thread stack that resumes at __sched_trampoline.
//
// Called by the scheduler internals.
//
// Do not use outside of this subsystem.
uintptr_t __sched_build_switch_frame(uintptr_t sp);

#endif // KERNEL_SCHED_SWITCH_H
