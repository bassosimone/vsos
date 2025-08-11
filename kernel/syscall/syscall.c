// File: kernel/sys/syscall.c
// Purpose: implement the syscall function
// SPDX-License-Identifier: MIT

#include <sys/errno.h>	 // for ENOSYS
#include <sys/syscall.h> // for SYS_write
#include <sys/types.h>	 // for uintptr_t

#include <unistd.h> // for syscall

intptr_t
syscall(uintptr_t num, uintptr_t a0, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5) {
	(void)a3;
	(void)a4;
	(void)a5;

	switch (num) {
	case SYS_read:
		return (intptr_t)read((int)a0, (char *)a1, (size_t)a2);

	case SYS_write:
		return (intptr_t)write((int)a0, (const char *)a1, (size_t)a2);

	default:
		return -ENOSYS;
	}
}
