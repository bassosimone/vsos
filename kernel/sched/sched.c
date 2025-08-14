// File: kernel/sched/thread.c
// Purpose: kernel thread scheduler
// SPDX-License-Identifier: MIT

#include <kernel/asm/asm.h>       // for cpu_sleep_until_interrupt
#include <kernel/clock/clock.h>   // for clock_tick_start
#include <kernel/core/assert.h>   // for KERNEL_ASSERT
#include <kernel/core/panic.h>    // for panic
#include <kernel/core/printk.h>   // for printk
#include <kernel/core/spinlock.h> // for struct spinlock
#include <kernel/exec/load.h>     // for struct load_program
#include <kernel/mm/vm.h>         // for struct vm_root_pt
#include <kernel/sched/sched.h>   // the subsystem's API
#include <kernel/sched/switch.h>  // switching threads
#include <kernel/trap/trap.h>     // for trap_restore_user_and_eret

#include <string.h>    // for memset
#include <sys/errno.h> // for EAGAIN
#include <sys/param.h> // for SCHED_MAX_THREADS

// The scheduler is not using this thread slot.
#define SCHED_THREAD_STATE_UNUSED 0

// The thread is currently runnable by the scheduler.
#define SCHED_THREAD_STATE_RUNNABLE 1

// The thread has explicitly stopped running.
#define SCHED_THREAD_STATE_EXITED 2

// The thread is blocked waiting for a specific event to happen.
#define SCHED_THREAD_STATE_BLOCKED 3

// The size in bytes of the statically-allocated stack.
#define SCHED_THREAD_STACK_SIZE 8192

// A process contains resources including threads.
//
// Currently empty since there are no resources shared by
// threads within the same userspace process.
//
// We will need to add here the file descriptors.
struct sched_process {};

// A schedulable thread of execution.
struct sched_thread {
	// The thread stack pointer.
	//
	// CAUTION: This field MUST be the first element of a
	// thread since the switch code assumes this.
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

	// The raw trap frame pointer, which points inside the stack.
	uintptr_t trapframe;

	// The thing the thread is blocked by.
	uint64_t blockedby;

	// The epoch when the thread was created.
	uint64_t epoch;

	// Back pointer to the owning process.
	//
	// NULL if we are a kernel thread.
	//
	// We use a `__` name to hint that one should use
	// accessors to safely access this field.
	struct sched_process *__proc;

	// Storage for allocating a proper process.
	//
	// This strategy is fine as long as we have a single
	// thread for each user process.
	struct sched_process __proc_storage;
};

// Ensure that the important invariant assumed by switching code is still valid.
static_assert(__builtin_offsetof(struct sched_thread, sp) == 0, "sp offset");

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

// Flag indicating we should reschedule
static uint64_t need_sched = 0;

// Number of ticks since the system has booted.
static volatile uint64_t jiffies = 0;

void sched_clock_init_irqs(void) {
	clock_tick_start();
}

void sched_clock_isr(void) {
	__atomic_fetch_add(&jiffies, 1, __ATOMIC_RELEASE);
	sched_thread_resume_all(SCHED_THREAD_WAIT_TIMER);
	clock_tick_rearm();
	__atomic_store_n(&need_sched, 1, __ATOMIC_RELEASE);
}

static inline bool __sched_should_reschedule(void) {
	return __atomic_exchange_n(&need_sched, 0, __ATOMIC_ACQUIRE) != 0;
}

static inline uint64_t __sched_jiffies(int memoryorder) {
	return __atomic_load_n(&jiffies, memoryorder);
}

void __sched_trampoline(void) {
	current->main(current->opaque);
	sched_thread_exit(0);
}

static inline void __sched_thread_stack_init(struct sched_thread *thread) {
	// See the stack as a uint8 aligned array
	uintptr_t sp = (uintptr_t)&thread->stack[SCHED_THREAD_STACK_SIZE];

	// Ensure the stack is 16 byte aligned
	KERNEL_ASSERT((sp & 0xF) == 0);

	// Use assembly magic to create the switch frame
	thread->sp = __sched_build_switch_frame(sp);
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

	// 3. for safety zero initialize the whole slot
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

	// 10. set the thread's epoch
	candidate->epoch = __sched_jiffies(__ATOMIC_RELAXED);

	// 11. return the thread ID.
	return candidate->id;
}

int64_t sched_thread_start(sched_thread_main_t *main, void *opaque, uint64_t flags) {
	// Ensure no-one can modify the thread global state while we're creating a thread
	spinlock_acquire(&lock);
	int64_t rv = __sched_thread_start_locked(main, opaque, flags);
	spinlock_release(&lock);
	return rv;
}

// Panic if we cannot get the process associated with the current thread.
static inline struct sched_process *must_get_process(struct sched_thread *thread) {
	KERNEL_ASSERT(thread->__proc != 0);
	return thread->__proc;
}

static void __sched_thread_process_main(void *opaque) {
	// 1. for sanity make sure we're running as a thread
	KERNEL_ASSERT(current != 0);

	// 2. initialize the process associated with this thread
	// and ensure it's possible to round trip this.
	current->__proc = &current->__proc_storage;
	struct sched_process *proc = must_get_process(current);
	KERNEL_ASSERT(proc == current->__proc);

	// 3. retrieve the user program we should execute.
	struct load_program *program = (struct load_program *)opaque;
	KERNEL_ASSERT(program != 0);

	// 4. call assembly code to initialize the trapframe
	// relative to the current thread stack. The idea here
	// is to simulate having taking a sync trap.
	current->trapframe = trap_create_process_frame(program->entry, program->root.table, program->stack_top);

	// 5. return to userspace. This is another geronimooooo case!
	trap_restore_user_and_eret(current->trapframe);

	// 6. we expect to never return here
	panic("trap_restore_user_and_eret should never return");
}

int64_t sched_process_start(struct load_program *program) {
	uint64_t flags = SCHED_THREAD_FLAG_JOINABLE | SCHED_THREAD_FLAG_PROCESS;
	return sched_thread_start(__sched_thread_process_main, (void *)program, flags);
}

// Loop forever yielding the CPU and then awaiting for interrupts.
[[noreturn]] static void __idle_main(void *unused) {
	(void)unused;
	// Just sleep without consuming resources and yield
	// the processor as soon as possible.
	for (;;) {
		sched_thread_yield();
		cpu_sleep_until_interrupt();
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
	printk("scheduler: created idle thread with ID: %lld\n", rv);
	KERNEL_ASSERT(rv >= 0);

	// Ensure the idle thread is accessible
	idle_thread = &threads[rv];

	// Manually set it as the currently running thread
	printk("scheduler: setting the idle thread as the current thread\n");
	current = idle_thread;

	// Manually switch to its execution context
	printk("scheduler: transferring control to the idle thread\n");
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

static inline void __sched_thread_yield(void) {
	// 1. Acquire the spinlock to prevent anyone else with messing with threads.
	spinlock_acquire(&lock);

	// 2. obtain the next thread we should run
	struct sched_thread *next = select_runnable();

	// 3. ensure it is not NULL
	KERNEL_ASSERT(next != 0);

	// 4. perform the context switch proper
	__unlock_and_switch_to(next);
}

void sched_thread_yield(void) {
	// Disable interrupts while switching
	local_irq_disable();

	// Perform the actual switch
	__sched_thread_yield();

	// Re-enable interrupts when done
	local_irq_enable();
}

[[noreturn]] void sched_thread_exit(void *retval) {
	// 0. make sure the current invariant holds otherwise someone is
	// calling sched_thread_exit before we have threads
	KERNEL_ASSERT(current != 0);

	// 1. ensure no-one can modify the threads state
	spinlock_acquire(&lock);

	// 2. set the return value
	current->retval = retval;

	// 3. mark the thread as zombie or exited
	if ((current->flags & SCHED_THREAD_FLAG_JOINABLE) != 0) {
		current->state = SCHED_THREAD_STATE_EXITED;
		events |= __SCHED_THREAD_WAIT_THREAD; // publish announcement
	} else {
		current->state = SCHED_THREAD_STATE_UNUSED;
	}

	// 4. allow others to access the thread
	spinlock_release(&lock);

	// 5. transfer the control to another thread
	sched_thread_yield();

	// 6. ensure that we don't arrive here
	panic("thread resumed execution after terminating");
}

int64_t sched_thread_join(int64_t tid, void **retvalptr) {
	KERNEL_ASSERT(current != 0);

	// Just avoid wasting time with out of bound thread IDs
	if (tid < 0 || tid >= SCHED_MAX_THREADS) {
		return -EINVAL;
	}

	// OK, let's bite the spinlock
	spinlock_acquire(&lock);

	uint64_t oepoch = 0;
	for (;;) {

		// Do not assume the thread state is stable
		// for example pthread_detach(self) can cause
		// a thread that was awaitable to vanish
		struct sched_thread *other = &threads[tid];
		bool isjoinable = (other->flags & SCHED_THREAD_FLAG_JOINABLE) != 0;
		oepoch = (oepoch == 0) ? other->epoch : oepoch;
		switch (other->state) {

		// Continue waiting for a blocked/running awaitable thread
		case SCHED_THREAD_STATE_BLOCKED:
		case SCHED_THREAD_STATE_RUNNABLE:
			if (!isjoinable) {
				spinlock_release(&lock);
				return -EINVAL;
			}

			// Await for *any* thread to terminate.
			//
			// Yeah, it does not scale well, but we need to start
			// from something simple don't we?
			spinlock_release(&lock);
			sched_thread_suspend(__SCHED_THREAD_WAIT_THREAD);
			spinlock_acquire(&lock);

			// Detect *sad* cases in which we have slept so much or
			// there was reaping contention and the actual thread
			// we were tracking already sleeps with the fishes
			if (oepoch != other->epoch) {
				spinlock_release(&lock);
				return -EINVAL;
			}
			continue;

		// OK, this is the case where we have fun
		case SCHED_THREAD_STATE_EXITED:
			KERNEL_ASSERT(isjoinable);                // must be the case
			*retvalptr = other->retval;               // transfer ownership
			other->state = SCHED_THREAD_STATE_UNUSED; // make a short funeral
			spinlock_release(&lock);
			return 0;

		// Maybe it detached itself and exited WTF
		default:
			spinlock_release(&lock);
			return -EINVAL;
		}
	}
}

[[noreturn]] void sched_return_to_user(uintptr_t raw_frame) {
	// Ensure we have current
	KERNEL_ASSERT(current != 0);

	// Save the raw frame of the thread we're going to suspend
	current->trapframe = raw_frame;

	// Check whether we should reschedule and switch if it's needed
	if (__sched_should_reschedule()) {
		__sched_thread_yield();
	}

	// Ensure we still have current
	KERNEL_ASSERT(current != 0);

	// Ensure we have a trap frame
	KERNEL_ASSERT(current->trapframe != 0);

	// TODO(bassosimone): ensure the trapframe is in the stack bounds?

	// The venerable `call/cc` is alive and fights alongside us
	trap_restore_user_and_eret(current->trapframe);

	// Just make sure we don't arrive here
	__builtin_unreachable();
}

void sched_thread_maybe_yield(void) {
	if (__sched_should_reschedule()) {
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

void __sched_thread_sleep(uint64_t jiffies) {
	uint64_t start = __sched_jiffies(__ATOMIC_RELAXED);
	for (;;) {
		sched_thread_suspend(SCHED_THREAD_WAIT_TIMER);
		uint64_t current = __sched_jiffies(__ATOMIC_RELAXED);
		if (current - start >= jiffies) {
			return;
		}
	}
}
