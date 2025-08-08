// File: libc/errno/errno.c
// Purpose: definition of the errno variable
// SPDX-License-Identifier: MIT

#include <kernel/sys/errno.h>

volatile int errno = 0;
