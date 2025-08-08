// File: kernel/sched/thread.h
// Purpose: kernel and user thread scheduler
// SPDX-License-Identifier: MIT
#ifndef KERNEL_SCHED_THREAD_H
#define KERNEL_SCHED_THREAD_H

#include <kernel/mm/types.h>
#include <kernel/sys/types.h>

// The scheduler is not using this thread slot.
#define SCHED_THREAD_STATE_UNUSED 0

// The thread is currently runnable by the scheduler.
#define SCHED_THREAD_STATE_RUNNABLE 1

// The thread has explicitly stopped running.
#define SCHED_THREAD_STATE_EXITED 2

// The size in bytes of the statically-allocated stack.
#define SCHED_THREAD_STACK_SIZE 8192

// The thread can be joined and must not be automatically reaped.
#define SCHED_THREAD_FLAG_JOINABLE (1 << 0)

// The type of a thread's main function.
typedef void(sched_thread_main_t)(void *opaque);

// A schedulable thread of execution.
struct sched_thread {
	// The thread stack pointer.
	uintptr_t sp;

	// The statically-allocated aligned stack.
	alignas(16) uint8_t stack[SCHED_THREAD_STACK_SIZE];

	// The thread ID.
	uint64_t id;

	// The thread state (one of SCHED_THREAD_STATE_xxx constants).
	uint64_t state;

	// The thread return value after it has exited.
	void *retval;

	// The main thread func.
	sched_thread_main_t *main;

	// The func argument.
	void *opaque;

	// Flags modifying the thread behavior (see SCHED_THREAD_FLAG_xxx).
	uint64_t flags;
};

// Starts a thread with the given main function and the given flags.
//
// When the flags are zero, the thread is detached.
//
// We recommend transferring ownership of the opaque pointer to the thread.
//
// Returns a negative errno value or the thread ID (>= 0).
int64_t sched_thread_start(sched_thread_main_t *main, void *opaque, uint64_t flags);

// Yields the CPU to another runnable thread.
//
// This function disables interrupts until the switching is complete
// so that the clock interrupt does not race with it.
//
// Returns when this thread becomes runnable again.
void sched_thread_yield(void);

// Internal implementation of yielding with interrupts being disabled.
//
// Do not call this outside of this module. The sched_thread_yield
// implementation is MD and disables interrupts before deferring to
// this function, which is MI, and does the context switch.
void __sched_thread_yield_without_interrupts(void);

// Terminates the thread execution and sets the return value.
//
// This function never returns.
[[noreturn]] void sched_thread_exit(void *retval);

// Machine-dependent context-switch implementation.
//
// The TL;DR is that we use the stack to store the prev context and then we
// load the new thread context from the stack and switch to it.
void __sched_switch(struct sched_thread *prev, struct sched_thread *next);

// Machine-dependent implementation of going idle for a while. Typically
// this should call `wfi` to save power until there's an interrupt.
void __sched_idle(void);

// Internal trampoline used to ensure we always call sched_thread_exit.
void __sched_trampoline(void);

// Machine-dependent code that sets the thread stack up such that the
// first execution of the thread resumes at __sched_trampoline.
void __sched_thread_stack_init(struct sched_thread *thread);

// Run the scheduler either switching to runnable threads or waiting for interrupts.
//
// This function must be called once at the end of the boot and assumes that
// no one has run other threads or called sched_thread_yield.
[[noreturn]] void sched_thread_run(void);

#endif // KERNEL_SCHED_THREAD_H
