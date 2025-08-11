// File: kernel/trap/trap_arm64.c
// Purpose: ARM64 code to handle traps.
// SPDX-License-Identifier: MIT

#include <kernel/trap/trap_arm64.h> // for struct trap_frame

#include <sys/types.h> // for uint64_t

#include <unistd.h> // for syscall

void __trap_ssr(struct trap_frame *frame, uint64_t esr, uint64_t far) {
	// TODO(bassosimone): we should use these
	(void)esr;
	(void)far;

	// We pass the syscall number as x8.
	//
	// Keep in sync with `./libc/unistd/syscall_arm64.c`.
	frame->x[0] = syscall(
	    frame->x[8], frame->x[0], frame->x[1], frame->x[2], frame->x[3], frame->x[4], frame->x[5]);
}
