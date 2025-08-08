// File: libc/errno/errno.h
// Purpose: stripped down errno.h header
// SPDX-License-Identifier: MIT
#ifndef LIBC_ERRNO_ERRNO_H
#define LIBC_ERRNO_ERRNO_H

#include <kernel/sys/errno.h>

extern volatile int errno;

#endif // LIBC_ERRNO_ERRNO_H
