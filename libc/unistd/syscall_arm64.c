// File: libc/unistd/syscall_arm64.c
// Purpose: syscall implementation for arm64
// SPDX-License-Identifier: MIT

#include <errno.h>     // for errno
#include <sys/types.h> // for uintptr_t
#include <unistd.h>    // for syscall

intptr_t
syscall(uintptr_t num, uintptr_t a0, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5) {
	// Keep in sync with `./kernel/trap/trap_arm64.c`
	register uintptr_t x8 __asm__("x8") = num;
	register intptr_t x0 __asm__("x0") = a0;
	register uintptr_t x1 __asm__("x1") = a1;
	register uintptr_t x2 __asm__("x2") = a2;
	register uintptr_t x3 __asm__("x3") = a3;
	register uintptr_t x4 __asm__("x4") = a4;
	register uintptr_t x5 __asm__("x5") = a5;
	__asm__ volatile("svc #0"
			 : "+r"(x0)
			 : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8)
			 : "memory", "cc");
	if (x0 < 0) {
		errno = (int)-x0;
		return -1;
	}
	return x0;
}
