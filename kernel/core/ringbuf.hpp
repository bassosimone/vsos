// File: kernel/core/ringbuf.hpp
// Purpose: Single-produce single-consumer ring buffer definition.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_CORE_RINGBUF_HPP
#define KERNEL_CORE_RINGBUF_HPP

#include <sys/types.h> // for size_t

// Single-producer single-consumer lockless ring buffer.
//
// A zero-initialized ringbuf is ready to use.
template <typename Type, size_t Size> struct ringbuf {
	// The actual buffer.
	Type buf[Size];

	// Pointer to the buffer head updated by the producer.
	size_t head;

	// Pointer to the buffer tail updated by the consumer.
	size_t tail;
};

// Push a specific value into the ring buffer.
template <typename Type, size_t Size> bool ringbuf_push(ringbuf<Type, Size> *rb, Type c) {
	// Ensure the size is reasonable.
	_Static_assert(Size > 0 && __builtin_popcount(Size) == 1);

	// Since we own the head we can be more relaxed.
	size_t head = __atomic_load_n(&rb->head, __ATOMIC_RELAXED);

	// Synchronize with the consumer releasing the tail.
	size_t tail = __atomic_load_n(&rb->tail, __ATOMIC_ACQUIRE);

	// If the buffer is full, reject the newest.
	if (head - tail >= Size) {
		return false;
	}

	// Append to the buffer.
	rb->buf[head & (Size - 1)] = c;

	// Synchronize with the producer acquiring the head.
	__atomic_store_n(&rb->head, head + 1, __ATOMIC_RELEASE);
	return true;
}

template <typename Type, size_t Size> bool ringbuf_pop(ringbuf<Type, Size> *rb, Type *c) {
	// Ensure the size is reasonable.
	_Static_assert(Size > 0 && __builtin_popcount(Size) == 1);

	// Since we own the tail we can be more relaxed.
	size_t tail = __atomic_load_n(&rb->tail, __ATOMIC_RELAXED);

	// Synchronize with the producer releasing the head.
	size_t head = __atomic_load_n(&rb->head, __ATOMIC_ACQUIRE);

	// The buffer is empty when tail has reached the head.
	if (tail == head) {
		return false;
	}

	// Read the next character from the ring buffer.
	*c = rb->buf[tail & (Size - 1)];

	// Synchronize with the consumer acquiring the tail.
	__atomic_store_n(&rb->tail, tail + 1, __ATOMIC_RELEASE);
	return true;
}

#endif // KERNEL_CORE_RINGBUF_HPP
