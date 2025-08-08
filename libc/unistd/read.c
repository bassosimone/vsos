// File: libc/unistd/read.c
// Purpose: read(2)
// SPDX-License-Identifier: MIT

#include <kernel/sys/syscall.h> // for SYS_read
#include <kernel/sys/types.h>	// for ssize_t
#include <libc/unistd/unistd.h> // for read

ssize_t read(int fd, char *buffer, size_t count) {
	return (ssize_t)syscall(SYS_read, (uintptr_t)fd, (uintptr_t)buffer, (uintptr_t)count, 0, 0, 0);
}
