// File: kernel/sys/errno.h
// Purpose: Define possible error values.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#ifndef KERNEL_SYS_ERRNO_H
#define KERNEL_SYS_ERRNO_H

// Resource temporarily unavailable.
#define EAGAIN 11

// Operation would block.
#define EWOULDBLOCK EAGAIN

#endif // KERNEL_SYS_ERRNO_H
