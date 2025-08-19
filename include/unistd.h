// File: unistd.h
// Purpose: stripped down unistd.h header
// SPDX-License-Identifier: MIT
#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <sys/cdefs.h> // for __BEGIN_DECLS
#include <sys/types.h> // for ssize_t

__BEGIN_DECLS

ssize_t read(int fd, char *buffer, size_t count) __NOEXCEPT;

ssize_t write(int fd, const char *buffer, size_t count) __NOEXCEPT;

intptr_t
syscall(uintptr_t num, uintptr_t a0, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5) __NOEXCEPT;

__END_DECLS

#endif // __UNISTD_H__
