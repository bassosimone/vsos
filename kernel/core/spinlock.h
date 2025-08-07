// File: kernel/core/spinlock.h
// Purpose: spinlock implementation.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_CORE_SPINLOCK_H
#define KERNEL_CORE_SPINLOCK_H

#include <kernel/sys/types.h>

// Contains a lock we will spin on.
struct spinlock {
	// The value controlling spin locking.
	volatile uint64_t value;
};

// Use this macro to intialize a static spinlock.
#define SPINLOCK_INITIALIZER {.value = 0}

// Initialize the spinlock before using it.
static inline void spinlock_init(struct spinlock *lock) {
	lock->value = 0;
}

// Continue spinning until we've acquired the lock.
static inline void spinlock_acquire(struct spinlock *lock) {
	while (__atomic_test_and_set(&lock->value, __ATOMIC_ACQUIRE)) {
		// 🌀🌀🌀🌀
	}
}

// Release the lock possibly enabling someone else to acquire it.
static inline void spinlock_release(struct spinlock *lock) {
	__atomic_clear(&lock->value, __ATOMIC_RELEASE);
}

#endif // KERNEL_CORE_SPINLOCK_H
