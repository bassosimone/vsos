// File: libc/string/memcpy.c
// Purpose: memcpy implementation from musl
// SPDX-License-Identifier: MIT
// Adapted from: https://git.musl-libc.org/cgit/musl/tree/src/string/memcpy.c

#include <string.h>

void *memcpy(void *dest, const void *src, size_t n) {
	unsigned char *d = dest;
	const unsigned char *s = src;
	for (; n; n--) {
		*d++ = *s++;
	}
	return dest;
}
