// File: kernel/boot/boot.c
// Purpose: Kernel entry point for ARM64
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/asm/arm64.h>	// for enable_fp_simd
#include <kernel/boot/layout.h> // for __stack_top, __bss, __bss_end
#include <kernel/core/panic.h>	// for panic
#include <kernel/core/printk.h> // for printk
#include <kernel/sys/types.h>	// for size_t
#include <kernel/tty/uart.h>	// for uart_init
#include <libc/string/string.h> // for memset

void __kernel_main(void) {
	// 0. enable FP/SIMD to avoid trapping on SIMD instructions
	enable_fp_simd();

	// 1. zero the BSS section
	memset(__bss, 0, (size_t)(__bss_end - __bss));

	// 2. initialize the serial console
	uart_init();

	// 3. for now just greet the user and die
	printk("Hello, world!\n");
	panic("Reached end of __kernel_main");
}

// This is the main assembly entry point of the kernel
__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
	__asm__ __volatile__("ldr x0, =__stack_top\n" // Load address of top of stack
			     "mov sp, x0\n"	      // Set SP
			     "bl __kernel_main\n"     // Branch with link to C main
			     :
			     :
			     : "x0" // Clobber x0
	);
}
