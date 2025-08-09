// File: kernel/sched/thread.c
// Purpose: kernel and user thread scheduler
// SPDX-License-Identifier: MIT

// TODO(bassosimone): maybe consider moving more assembly code
// related to context switching into the scheduler. After all, it
// seems scheduler code more than boot code.

#include <kernel/boot/irq.h>	  // for irq_restore_user_and_eret
#include <kernel/core/assert.h>	  // for KERNEL_ASSERT
#include <kernel/core/panic.h>	  // for panic
#include <kernel/core/spinlock.h> // for struct spinlock
#include <kernel/sched/clock.h>	  // for __sched_clock_init
#include <kernel/sched/thread.h>  // for sched_thread, etc.
#include <kernel/sys/errno.h>	  // for EAGAIN
#include <kernel/sys/param.h>	  // for SCHED_MAX_THREADS
#include <libc/string/string.h>	  // for memset

// Statically allocated list of threads.
static struct sched_thread threads[SCHED_MAX_THREADS];

// The thread that is currently running or zero if no thread is running.
//
// Note: this gets initialized to the idle thread when we try to switch the first time.
static struct sched_thread *current = 0;

// Spinlock protecting access to the threads.
static struct spinlock lock = SPINLOCK_INITIALIZER;

// The idle thread implementation.
//
// This is auto-initialized when we try to switch the first time.
static struct sched_thread *idle_thread = 0;

// ID used by the scheduler to try and be fair.
static size_t fair_id = 0;

// List of pending events since the last schedule occurred.
static uint64_t events = 0;

void __sched_trampoline(void) {
	current->main(current->opaque);
	sched_thread_exit(0);
}

// Assumption: the caller has acquired the spinlock
static int64_t __sched_thread_start_locked(sched_thread_main_t *main, void *opaque, uint64_t flags) {
	// 1. find thread that is currently unused
	size_t idx = 0;
	struct sched_thread *candidate = 0;
	for (; idx < SCHED_MAX_THREADS; idx++) {
		candidate = &threads[idx];
		if (candidate->state == SCHED_THREAD_STATE_UNUSED) {
			break;
		}
		candidate = 0;
	}

	// 2. handle the case where we cannot create a new thread.
	if (candidate == 0) {
		return -EAGAIN;
	}

	// 3. just in case zero the thread including its stack
	memset(candidate, 0, sizeof(*candidate));

	// 4. initialize the thread stack invoking MD code
	candidate->sp = (uintptr_t)&candidate->stack[sizeof(candidate->stack)];
	__sched_thread_stack_init(candidate);

	// 5. initialize the thread ID
	candidate->id = idx;

	// 6. initialize the thread state and make it runnable.
	candidate->state = SCHED_THREAD_STATE_RUNNABLE;

	// 7. initialize the thread retval.
	candidate->retval = 0;

	// 8. initialize the thread main func and argument.
	candidate->main = main;
	candidate->opaque = opaque;

	// 9. copy the flags.
	candidate->flags = flags;

	// 10. return the thread ID.
	return candidate->id;
}

int64_t sched_thread_start(sched_thread_main_t *main, void *opaque, uint64_t flags) {
	// Ensure no-one can modify the thread global state while we're creating a thread
	spinlock_acquire(&lock);
	int64_t rv = __sched_thread_start_locked(main, opaque, flags);
	spinlock_release(&lock);
	return rv;
}

// Loop forever yielding the CPU and then awaiting for interrupts.
[[noreturn]] static void __idle_main(void *unused) {
	(void)unused;

	// Start the pre-emptive scheduler
	__sched_clock_init();

	// Just sleep without consuming resources and yield
	// the processor as soon as possible.
	for (;;) {
		sched_thread_yield();
		__sched_idle();
	}
}

// Helper function to unlock the spinlock and switch current to next.
static void __unlock_and_switch_to(struct sched_thread *next) {
	// 1. Save the previous thread
	struct sched_thread *prev = current;

	// 2. Update current
	current = next;

	// 3. Release the spinlock before switching
	spinlock_release(&lock);

	// 4. do not perform any context switching if the two threads are equal
	if (prev == next) {
		return;
	}

	// 5. check for invariant assumed by __sched_switch
	static_assert(__builtin_offsetof(struct sched_thread, sp) == 0, "sp must be at offset 0");

	// 6. Use MD code to switch context
	__sched_switch(prev, next);
}

[[noreturn]] void sched_thread_run(void) {
	// Manually instantiate the idle thread
	int64_t rv = sched_thread_start(__idle_main, /* opaque */ 0, /* flags */ 0);
	KERNEL_ASSERT(rv >= 0);

	// Ensure the idle thread is accessible
	idle_thread = &threads[rv];

	// Manually set it as the currently running thread
	current = idle_thread;

	// Manually switch to its execution context
	__sched_switch(0, idle_thread);
	panic("unreachable");
}

// Function that selects the next thread to run or the idle thread.
//
// Must be invoked while holding the spinlock.
static struct sched_thread *select_runnable(void) {
	// 1. ensure we have a idle thread
	KERNEL_ASSERT(idle_thread != 0);

	// 2. ensure we have a current thread
	KERNEL_ASSERT(current != 0);

	// 3. grab the list of events and clear it
	uint64_t channels = events;
	events = 0;

	// 4. Switch to a runnable thread using a ~fair round-robin scheduling.
	for (size_t idx = 0; idx < SCHED_MAX_THREADS; idx++) {
		// 4.1. get the next thread we should consider for running.
		struct sched_thread *next = &threads[fair_id];
		fair_id = (fair_id == SCHED_MAX_THREADS - 1) ? 0 : fair_id + 1;

		// 4.2. avoid giving CPU time to the idle thread.
		if (next == idle_thread) {
			continue;
		}

		// 4.3. see whether we need to wakeup this specific thread.
		if (next->state == SCHED_THREAD_STATE_BLOCKED && (next->blockedby & channels) != 0) {
			next->state = SCHED_THREAD_STATE_RUNNABLE;
			next->blockedby = 0;
			/* FALLTHROUGH */
		}

		// 4.4. skip threads that are not marked as runnable.
		if (next->state != SCHED_THREAD_STATE_RUNNABLE) {
			continue;
		}

		// 4.5. return the candidate.
		return next;
	}

	// 5. if we end up here, we return the idle thread.
	return idle_thread;
}

void __sched_thread_yield_without_interrupts(void) {
	// 1. Acquire the spinlock to prevent anyone else with messing with threads.
	spinlock_acquire(&lock);

	// 2. obtain the next thread we should run
	struct sched_thread *next = select_runnable();

	// 3. ensure it is not NULL
	KERNEL_ASSERT(next != 0);

	// 4. perform the context switch proper
	__unlock_and_switch_to(next);
}

[[noreturn]] void sched_thread_exit(void *retval) {
	// 0. make sure the current invariant holds otherwise someone is
	// calling sched_thread_exit before we have threads
	KERNEL_ASSERT(current != 0);

	// 1. ensure no-one can modify the threads state
	spinlock_acquire(&lock);

	// 2. set the return value and mark the thread as exited
	bool needsjoin = (current->flags & SCHED_THREAD_FLAG_JOINABLE) != 0;
	current->retval = retval;
	current->state = needsjoin ? SCHED_THREAD_STATE_EXITED : SCHED_THREAD_STATE_UNUSED;

	// 3. allow others to access the thread
	spinlock_release(&lock);

	// 4. transfer the control to another thread
	sched_thread_yield();

	// 5. ensure that we don't arrive here
	panic("thread resumed execution after terminating");
}

[[noreturn]] void sched_return_to_user(uintptr_t raw_frame) {
	// Ensure we have current
	KERNEL_ASSERT(current != 0);

	// Save the raw frame of the thread we're going to suspend
	current->trapframe = raw_frame;

	// Check whether we should reschedule and switch if it's needed
	if (sched_should_reschedule()) {
		__sched_thread_yield_without_interrupts();
	}

	// Ensure we still have current
	KERNEL_ASSERT(current != 0);

	// Ensure we have a trap frame
	KERNEL_ASSERT(current->trapframe != 0);

	// TODO(bassosimone): ensure the trapframe is in the stack bounds?

	// The venerable `call/cc` is alive and fights alongside us
	irq_restore_user_and_eret(current->trapframe);

	// Just make sure we don't arrive here
	__builtin_unreachable();
}

void sched_thread_maybe_yield(void) {
	if (sched_should_reschedule()) {
		sched_thread_yield();
	}
}

void sched_thread_suspend(uint64_t channels) {
	KERNEL_ASSERT(current != 0);
	current->state = SCHED_THREAD_STATE_BLOCKED;
	current->blockedby = channels;
	sched_thread_yield();
}

void sched_thread_resume_all(uint64_t channels) {
	spinlock_acquire(&lock);
	events |= channels;
	spinlock_release(&lock);
}
