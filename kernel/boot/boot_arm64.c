// File: kernel/boot/boot_arm64.c
// Purpose: Kernel entry point for ARM64
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/asm/arm64.h> // for enable_fp_simd
#include <kernel/boot/boot.h> // whole sybsystem API

[[noreturn]] void kernel_main(void) {
	// Avoid traps when using FP/SIMD in the kernel
	enable_fp_simd();

	// Defer to the MI code
	__kernel_main();
}

__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
	__asm__ __volatile__("ldr x0, =__stack_top\n" // Load address of top of stack
			     "mov sp, x0\n"	      // Set SP
			     "bl kernel_main\n"	      // Branch with link to C main
			     :
			     :
			     : "x0" // Clobber x0
	);
}
