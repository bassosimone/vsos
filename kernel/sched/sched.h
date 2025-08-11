// File: kernel/sched/sched.h
// Purpose: Public and protected scheduler API
// SPDX-License-Identifier: MIT
#ifndef KERNEL_SCHED_SCHED_H
#define KERNEL_SCHED_SCHED_H

#include <sys/param.h> // for HZ
#include <sys/types.h> // for uint64_t

// Interrupt handler for the scheduler clock.
//
// Called from the generic IRQ handler.
//
// Will must acknowledge the IRQ, re-arm the timer, and
// perform all the related scheduler bookkeping.
void sched_clock_irq(void);

// Initialize the timer to interrupt every HZ.
//
// Called by the IRQ subsystem.
//
// Do not use outside of this subsystem.
void sched_clock_init_irq(void);

// Returns true if we should trigger a reschedule.
//
// Called by the scheduler internals.
//
// Do not use outside of this subsystem.
bool __sched_should_reschedule(void);

// Returns the current value of the scheduler jiffies.
//
// The jiffies count the number of clock interrupts.
//
// The `memoryorder` argument specifies the atomic memory
// order for accessing the underlying variable.
//
// From normal kernel thread context `__ATOMIC_RELAXED`
// is sufficient. From IRQ or SMP contexts, instead, one
// should use the `__ATOMIC_ACQUIRE` constraint.
uint64_t sched_jiffies(int memoryorder);

// The thread can be joined and must not be automatically reaped.
#define SCHED_THREAD_FLAG_JOINABLE (1 << 0)

// The type of the main function implementing a kernel thread.
typedef void(sched_thread_main_t)(void *opaque);

// Starts a thread with the given main function and the given flags.
//
// When `main` returns, the thread terminates.
//
// You SHOULD transfer ownership of `opaque` to the thread.
//
// The zero flags create a detached thread. The SCHED_THREAD_FLAG_JOINABLE flag
// creates a thread that you must explicitly join.
//
// Returns a negative errno value or the thread ID (>= 0).
int64_t sched_thread_start(sched_thread_main_t *main, void *opaque, uint64_t flags);

// Yields the CPU to another runnable thread.
//
// This function disables interrupts until the switching is complete
// so that the clock interrupt does not race with it.
//
// Returns when this thread becomes runnable again.
//
// If the thread exits, this function will never return.
void sched_thread_yield(void);

// Like sched_thread_yield but without disabling interrupts.
//
// Called by the scheduler internals.
//
// Do not use outside of this subsystem.
void __sched_thread_yield(void);

// Terminates the thread execution and sets the return value.
//
// The `retval` memory ownership is transferred from the thread to
// whoever will join it and read the return value.
//
// This function never returns, rather it is a cooperative
// scheduling point that schedules a new thread.
[[noreturn]] void sched_thread_exit(void *retval);

// Waits for the given thread to terminate.
//
// Fails immediately returning `-EINVAL` if the given thread ID is
// invalid or the thread is actually not joinable.
//
// In all other cases, the current thread is suspended until the
// thread with the given ID calls sched_pthread_exit.
//
// When the current thread resumes, it collects the retval, of which
// it takes memory ownership, and the other thread is reaped, while
// sched_thread_join returns `0` to its caller.
//
// When the other thread has already terminated, this function may
// potentially sleep awaiting for the notification depending on what
// happens inside the scheduler and within the IRQs.
int64_t sched_thread_join(int64_t tid, void **retvalptr);

// Opaque representation of a kernel thread.
struct sched_thread;

// Machine-dependent context-switch implementation.
//
// "You are not expected to understand this".
//
// Called by the scheduler internals.
//
// Do not use outside of this subsystem.
void __sched_switch(struct sched_thread *prev, struct sched_thread *next);

// Put the CPU in low-power mode until an interrupt occurs.
//
// Called by the scheduler internals.
//
// Do not use outside of this subsystem.
void __sched_idle(void);

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

// Switch to the first runnable thread and never return.
//
// The control will constantly switch between runnable threads.
//
// Called by the late boot process.
//
// Make sure this function is called just once.
[[noreturn]] void sched_thread_run(void);

// Return to userspace possibly switching to another process.
//
// Called right before returning to userspace, typically by
// the IRQ-from-userspace or syscall ASM handling code.
//
// The frame argument points to the (machine-dependent)
// complete trap frame necessary to swap context.
//
// This function never returns. Rather it saves the current
// trapframe, loads the trapframe of a thread previously
// suspended by returning to userspace, and issues a return
// from userspace from such a context.
[[noreturn]] void sched_return_to_user(uintptr_t raw_frame);

// Call this function from kernel threads to ensure that the scheduler
// has a chance to schedule another process when needed.
//
// The general idea is that the clock ticker sets an atomic flag
// indicating the need to reschedule and device interrupt handlers
// may also set the same or similar flags indicating that someone
// needs to wakeup to perform some action. However, especially when
// servicing interrupts at kernel level, it is not viable to run
// the scheduler directly. Instead, by sprinkling our kernel threads
// code with invocations of this function, we naturally provide the
// ability to change the kernel focus and do other stuff.
//
// The rule of thumb of using this function is the following:
//
// 1. If the kernel thread function does not loop, invoke this
// function just once at the very beginning.
//
// 2. If the kernel thread function does loop, invoke this
// function inside the loop itself.
//
// The idle thread should not use this function since it runs
// a loop that blocks on interrupts, so each interrupt will
// already interrupt it without the need to call this. It should
// instead invoke sched_thread_yield unconditionally.
//
// Interrupt handlers MUST NOT use this function. This function
// MUST NOT be used inside of critical sections.
//
// Functions that call this function should document themselves
// as cooperative synchronization points.
void sched_thread_maybe_yield(void);

// The thread is waiting for the UART to become readable.
#define SCHED_THREAD_WAIT_UART_READABLE (1 << 0)

// The thread is waiting for the UART to become writable.
#define SCHED_THREAD_WAIT_UART_WRITABLE (1 << 1)

// The thread is waiting on a timeer to expire.
#define SCHED_THREAD_WAIT_TIMER (1 << 2)

// The thread is waiting on another thread to terminate.
//
// Used by the scheduler internals.
//
// Do not use outside of this subsystem.
#define __SCHED_THREAD_WAIT_THREAD (1 << 3)

// This function suspends the current thread until one of the
// given channels (e.g., SCHED_THREAD_WAIT_UART_READABLE) becomes
// available. Channels are a bitmask of possible event sources.
//
// This function MUST be called whenever it would be safe to
// call the sched_thread_maybe_yield function.
void sched_thread_suspend(uint64_t channels);

// This function updates the bitmask of events occurred since
// the last scheduler invocation. The next cooperative multitasking
// point inside the kernel will unblock all the threads blocked by
// the events accumulated inside the bitmask and clear it.
//
// Upon scheduling, the threads will run again. Spurious wakeups
// are possible. Therefore, a thread will need to check whether the
// condition it was blocked on was satisfied or not. If not, the
// thread should suspend itself again.
//
// This function is typically called from interrupt context.
void sched_thread_resume_all(uint64_t channels);

// Put the given thread to sleep for the given amount of jiffies.
//
// Safe to call whenever you can call sched_thread_suspend.
void __sched_thread_sleep(uint64_t jiffies);

// Put the current thread to sleep for the given amount of nanoseconds.
//
// Safe to call whenever you can call sched_thread_suspend.
static inline void sched_thread_nanosleep(uint64_t nanosec) {
	return __sched_thread_sleep((nanosec * HZ) / (1000 * 1000 * 1000));
}

// Put the current thread to sleep for the given amount of milliseconds.
//
// Safe to call whenever you can call sched_thread_suspend.
static inline void sched_thread_millisleep(uint64_t millisec) {
	return __sched_thread_sleep((millisec * HZ) / 1000);
}

// Put the current thread to sleep for the given amount of seconds.
//
// Safe to call whenever you can call sched_thread_suspend.
static inline void sched_thread_sleep(uint64_t sec) {
	return __sched_thread_sleep(sec * HZ);
}

#endif // KERNEL_SCHED_SCHED_H
