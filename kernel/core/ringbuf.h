// File: kernel/core/ringbuf.h
// Purpose: simple ring buffer definition
// SPDX-License-Identifier: MIT
#ifndef KERNEL_CORE_RINGBUF_H
#define KERNEL_CORE_RINGBUF_H

#include <sys/types.h>

// Size of a ringbuf buffer.
#define RINGBUF_SIZE 256

// Mask for obtaining head and tail.
#define RINGBUF_MASK (RINGBUF_SIZE - 1)

// Ensure the size is a power of two.
static_assert(__builtin_popcount(RINGBUF_SIZE) == 1);

// Single-producer single-consumer lockless ring buffer.
//
// A zero-initialized ringbuf is ready to use.
struct ringbuf {
	// The actual buffer.
	uint16_t buf[RINGBUF_SIZE];

	// Pointer to the buffer head updated by the producer.
	size_t head;

	// Pointer to the buffer tail updated by the consumer.
	size_t tail;
};

// Push a value into the ring buffer.
static inline bool ringbuf_push(struct ringbuf *rb, uint16_t c) {
	// Since we own the head we can be more relaxed.
	size_t head = __atomic_load_n(&rb->head, __ATOMIC_RELAXED);

	// Synchronize with the consumer releasing the tail.
	size_t tail = __atomic_load_n(&rb->tail, __ATOMIC_ACQUIRE);

	// If the buffer is full, reject the newest.
	if (head - tail >= RINGBUF_SIZE) {
		return false;
	}

	// Append to the buffer.
	rb->buf[head & RINGBUF_MASK] = c;

	// Synchronize with the producer acquiring the head.
	__atomic_store_n(&rb->head, head + 1, __ATOMIC_RELEASE);
	return true;
}

static inline bool ringbuf_pop(struct ringbuf *rb, uint16_t *c) {
	// Since we own the tail we can be more relaxed.
	size_t tail = __atomic_load_n(&rb->tail, __ATOMIC_RELAXED);

	// Synchronize with the producer releasing the head.
	size_t head = __atomic_load_n(&rb->head, __ATOMIC_ACQUIRE);

	// The buffer is empty when tail has reached the head.
	if (tail == head) {
		return false;
	}

	// Read the next character from the ring buffer.
	*c = rb->buf[tail & RINGBUF_MASK];

	// Synchronize with the consumer acquiring the tail.
	__atomic_store_n(&rb->tail, tail + 1, __ATOMIC_RELEASE);
	return true;
}

#endif // KERNEL_CORE_RINGBUF_H
