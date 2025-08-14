// File: include/sys/errno.h
// Purpose: Define possible error values.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef __SYS_ERRNO_H__
#define __SYS_ERRNO_H__

// We try to use the same numbers used by Linux

// Input/output error.
#define EIO 5

// Exec format error.
#define ENOEXEC 8

// Bad file descriptor.
#define EBADF 9

// Resource temporarily unavailable.
#define EAGAIN 11

// Out of memory.
#define ENOMEM 12

// Operation would block.
#define EWOULDBLOCK EAGAIN

// Invalid argument.
#define EINVAL 22

// Function not implemented.
#define ENOSYS 38

#endif // __SYS_ERRNO_H__
