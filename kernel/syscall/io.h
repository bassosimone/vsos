// File: kernel/syscall/io.h
// Purpose: copy_from_user and copy_to_user
// SPDX-License-Identifier: MIT
#ifndef KERNEL_SYSCALL_IO_H
#define KERNEL_SYSCALL_IO_H

#include <sys/types.h> // for ssize_t

// Copies up to count bytes from the user pointer src to the kernel pointer dst.
//
// Returns the number of bytes copied or a negative errno on failure.
//
// The count buffer should be the smaller of the size of the two buffers.
ssize_t copy_from_user(char *dst, const char *src, size_t count);

// Copies up to count bytes from the kernel points src to the user pointer dst.
//
// Returns the number of bytes copied or a negative errno on failure.
//
// The count buffer should be the smaller of the size of the two buffers.
ssize_t copy_to_user(char *dst, const char *src, size_t count);

#endif // KERNEL_SYSCALL_IO_H
