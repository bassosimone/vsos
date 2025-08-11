// File: libc/errno/errno.c
// Purpose: definition of the errno variable
// SPDX-License-Identifier: MIT

#include <errno.h>

volatile int errno = 0;
