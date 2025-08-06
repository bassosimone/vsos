// File: libc/string/string.h
// Purpose: stripped down string.h header
// SPDX-License-Identifier: MIT
#ifndef LIBC_STRING_STRING_H
#define LIBC_STRING_STRING_H

#include <kernel/sys/types.h>

void *memset(void *dest, int c, size_t n);

int strncmp(const char *_l, const char *_r, size_t n);

#endif // LIBC_STRING_STRING_H
