// File: kernel/sched/sched.h
// Purpose: Public scheduler API
// SPDX-License-Identifier: MIT
#ifndef KERNEL_SCHED_SCHED_H
#define KERNEL_SCHED_SCHED_H

#include <kernel/exec/load.h> // for struct load_program

#include <sys/cdefs.h> // for __BEGIN_DECLS
#include <sys/param.h> // for HZ
#include <sys/types.h> // for __status_t

__BEGIN_DECLS

// Interrupt service routine for the scheduler clock.
//
// Called from the generic IRQ handler.
//
// Will must acknowledge the IRQ, re-arm the timer, and
// perform all the related scheduler bookkeping.
void sched_clock_isr(void) __NOEXCEPT;

// Initialize the timer to interrupt every HZ.
//
// Called by the IRQ subsystem.
//
// Do not use outside of this subsystem.
void sched_clock_init_irqs(void) __NOEXCEPT;

// The thread can be joined and must not be automatically reaped.
#define SCHED_THREAD_FLAG_JOINABLE (1 << 0)

// The thread is attached to a user process instance.
#define SCHED_THREAD_FLAG_PROCESS (1 << 1)

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
// Returns a negative errno value on failure and zero on success.
//
// The *tid return argument must be nonnull and will contain the thread ID.
__status_t sched_thread_start(__thread_id_t *tid, sched_thread_main_t *main, void *opaque, __flags32_t flags) __NOEXCEPT;

// Force the current thread to return to userspace and execute the given program.
//
// The current thread will setup the process resources and prepare a synthetic
// trapframe and then return to userspace using `ERET`.
//
// From then on, the kernel thread will be the permanent parent of the process.
//
// Note that, when we reach this point of the `execve` syscall, we have basically
// already have parsed the ELF, so we known we can execute something, and what
// would prevent the process from starting is lack of system resources.
//
// Also, the current thread is marked as a user process, so that tools like `ps`
// report that this is a user process with the given ID.
//
// Additionally, the thread is marked as joinable since processes are joinable
// by default, including init, for which we should panic if it returns.
[[noreturn]] void sched_process_exec(struct load_program *program) __NOEXCEPT;

// Yields the CPU to another runnable thread.
//
// This function disables interrupts until the switching is complete
// so that the clock interrupt does not race with it.
//
// Returns when this thread becomes runnable again.
//
// If the thread exits, this function will never return.
void sched_thread_yield(void) __NOEXCEPT;

// Terminates the thread execution and sets the return value.
//
// The `retval` memory ownership is transferred from the thread to
// whoever will join it and read the return value.
//
// This function never returns, rather it is a cooperative
// scheduling point that schedules a new thread.
[[noreturn]] void sched_thread_exit(void *retval) __NOEXCEPT;

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
__status_t sched_thread_join(__thread_id_t tid, void **retvalptr) __NOEXCEPT;

// Opaque representation of a kernel thread.
struct sched_thread;

// Get the current process page table.
//
// Returns a negative errno on failure and zero on success.
//
// Panics if table is 0.
//
// On failure, initializes *table to a zero value.
__status_t sched_current_process_page_table(struct vm_root_pt *table) __NOEXCEPT;

// Switch to the first runnable thread and never return.
//
// The control will constantly switch between runnable threads.
//
// Called by the late boot process.
//
// Make sure this function is called just once.
[[noreturn]] void sched_thread_run(void) __NOEXCEPT;

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
[[noreturn]] void sched_return_to_user(uintptr_t raw_frame) __NOEXCEPT;

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
void sched_thread_maybe_yield(void) __NOEXCEPT;

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

// Type representing channels on which a kernel thread may suspend.
//
// This type is 64-bit wide regardless of the word size so that, with the current
// bitmask, approach, we can model at least 64 event sources.
typedef uint64_t sched_channels_t;

// This function suspends the current thread until one of the
// given channels (e.g., SCHED_THREAD_WAIT_UART_READABLE) becomes
// available. Channels are a bitmask of possible event sources.
//
// This function MUST be called whenever it would be safe to
// call the sched_thread_maybe_yield function.
void sched_thread_suspend(sched_channels_t channels) __NOEXCEPT;

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
void sched_thread_resume_all(sched_channels_t channels) __NOEXCEPT;

// Put the given thread to sleep for the given amount of jiffies.
//
// Safe to call whenever you can call sched_thread_suspend.
//
// Has a private-like name because usually you want to use higher-level APIs.
void __sched_thread_sleep(__duration64_t jiffies) __NOEXCEPT;

// Put the current thread to sleep for the given amount of nanoseconds.
//
// Safe to call whenever you can call sched_thread_suspend.
static inline void sched_thread_nanosleep(__duration64_t nanosec) __NOEXCEPT {
	return __sched_thread_sleep((nanosec * HZ) / (1000 * 1000 * 1000));
}

// Put the current thread to sleep for the given amount of milliseconds.
//
// Safe to call whenever you can call sched_thread_suspend.
static inline void sched_thread_millisleep(__duration64_t millisec) __NOEXCEPT {
	return __sched_thread_sleep((millisec * HZ) / 1000);
}

// Put the current thread to sleep for the given amount of seconds.
//
// Safe to call whenever you can call sched_thread_suspend.
static inline void sched_thread_sleep(__duration64_t sec) __NOEXCEPT {
	return __sched_thread_sleep(sec * HZ);
}

__END_DECLS

#endif // KERNEL_SCHED_SCHED_H
