// File: kernel/boot/boot_arm64.c
// Purpose: Kernel entry point for ARM64
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/asm/arm64.h>	   // for enable_fp_simd
#include <kernel/boot/irq_arm64.h> // for gicv2_init
#include <kernel/boot/layout.h>	   // for __stack_top, __bss, __bss_end
#include <kernel/core/panic.h>	   // for panic
#include <kernel/core/printk.h>	   // for printk
#include <kernel/mm/vmap.h>	   // mm_init
#include <kernel/sched/clock.h>	   // for sched_clock_init
#include <kernel/sched/thread.h>   // for sched_thread_run
#include <kernel/sys/types.h>	   // for size_t
#include <kernel/tty/uart.h>	   // for uart_init
#include <libc/string/string.h>	   // for memset

static void thread_hello(void *opaque) {
	(void)opaque;
	printk("Hello, world\n");
	sched_thread_yield();
	printk("Hello, world\n");
	sched_thread_yield();
	printk("Hello, world\n");
	sched_thread_yield();
}

static void thread_goodbye(void *opaque) {
	(void)opaque;
	printk("Goodbye, world\n");
	sched_thread_yield();
	printk("Goodbye, world\n");
	sched_thread_yield();
	printk("Goodbye, world\n");
	sched_thread_yield();
}

void __kernel_main(void) {
	// 0. enable FP/SIMD to avoid trapping on SIMD instructions
	enable_fp_simd();

	// 1. zero the BSS section
	memset(__bss, 0, (size_t)(__bss_end - __bss));

	// 2. initialize the serial console
	uart_init();
	printk("initialized uart\n");

	// 3. intialize the GICv2
	printk("initializing gicv2...\n");
	gicv2_init();
	printk("initializing gicv2... ok\n");

	// 4. initialize the memory manager
	printk("initializing mm...\n");
	mm_init();
	printk("initializing mm... ok\n");

	// 5. create a thread for saying hello to the world
	int64_t tid = sched_thread_start(thread_hello, /* opaque */ 0, /* flags */ 0);
	printk("started hello thread: %d\n", tid);

	// 6. create a thread for saying goodbye to the world
	tid = sched_thread_start(thread_goodbye, /* opaque */ 0, /* flags */ 0);
	printk("started goodbye thread: %d\n", tid);

	// 7. run the thread scheduler
	printk("starting thread scheduler...\n");
	sched_thread_run();
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
