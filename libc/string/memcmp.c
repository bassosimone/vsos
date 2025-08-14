// File: libc/string/memcmp.c
// Purpose: memcmp implementation from musl
// SPDX-License-Identifier: MIT
// Adapted from: https://git.musl-libc.org/cgit/musl/tree/src/string/memcmp.c

#include <string.h>

int memcmp(const void *vleft, const void *vright, size_t count) {
	const unsigned char *left = vleft, *right = vright;
	for (; count && *left == *right; count--, left++, right++) {
		;
	}
	return count ? *left - *right : 0;
}
