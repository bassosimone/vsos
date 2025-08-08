// File: libc/unistd/write.c
// Purpose: write(2)
// SPDX-License-Identifier: MIT

#include <kernel/sys/syscall.h> // for SYS_write
#include <kernel/sys/types.h>	// for ssize_t
#include <libc/unistd/unistd.h> // for write

ssize_t write(int fd, const char *buffer, size_t count) {
	return (ssize_t)syscall(SYS_write, (uintptr_t)fd, (uintptr_t)buffer, (uintptr_t)count, 0, 0, 0);
}
