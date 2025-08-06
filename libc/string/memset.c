// File: libc/string/memset.c
// Purpose: memset implementation from musl
// SPDX-License-Identifier: MIT
// Adapted from: https://git.musl-libc.org/cgit/musl/tree/src/string/memset.c

#include <libc/string/string.h>

void *memset(void *dest, int c, size_t n) {
	unsigned char *s = dest;
	for (; n; n--, s++) {
		*s = c;
	}
	return dest;
}
