// File: include/string.h
// Purpose: stripped down string.h header
// SPDX-License-Identifier: MIT
#ifndef __STRING_H__
#define __STRING_H__

#include <sys/cdefs.h> // for __BEGIN_DECLS
#include <sys/types.h> // for size_t

__BEGIN_DECLS

int memcmp(const void *vleft, const void *vright, size_t count) __NOEXCEPT;

void *memcpy(void *dest, const void *src, size_t n) __NOEXCEPT;

void *memset(void *dest, int c, size_t n) __NOEXCEPT;

int strncmp(const char *left, const char *right, size_t count) __NOEXCEPT;

// Nonstandard function that zeroes memory that is not aligned without
// causing data aborts caused by SIMD instructions.
static inline void __bzero_unaligned(volatile void *data, size_t count) __NOEXCEPT {
	volatile unsigned char *base = data;
	for (size_t idx = 0; idx < count; idx++) {
		base[idx] = 0;
	}
}

// The bzero BSD extension makes the code easier to audit.
static inline void __bzero(void *data, size_t count) __NOEXCEPT {
	memset(data, 0, count);
}

__END_DECLS

#endif // __STRING__
