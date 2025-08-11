// File: include/string.h
// Purpose: stripped down string.h header
// SPDX-License-Identifier: MIT
#ifndef __STRING_H__
#define __STRING_H__

#include <sys/types.h>

void *memcpy(void *dest, const void *src, size_t n);

void *memset(void *dest, int c, size_t n);

int strncmp(const char *_l, const char *_r, size_t n);

#endif // __STRING__
