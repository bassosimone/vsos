// File: libc/string/memset.c
// Purpose: memset implementation from musl
// SPDX-License-Identifier: MIT
// Adapted from: https://git.musl-libc.org/cgit/musl/tree/src/string/strncmp.c

#include <string.h>

int strncmp(const char *_l, const char *_r, size_t n) {
	const unsigned char *l = (void *)_l, *r = (void *)_r;
	if (!n--) {
		return 0;
	}
	for (; *l && *r && n && *l == *r; l++, r++, n--) {
		;
	}
	return *l - *r;
}
