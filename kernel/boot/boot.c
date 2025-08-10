// File: kernel/boot/boot_arm64.c
// Purpose: Kernel entry point for ARM64
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/boot/boot.h>	// whole subsystem API
#include <kernel/core/panic.h>	// for panic
#include <kernel/core/printk.h> // for printk
#include <kernel/irq/irq.h>	// for irq_init
#include <kernel/mm/vmap.h>	// for mm_init
#include <kernel/sched/sched.h> // whole subsystem API
#include <kernel/sys/param.h>	// for HZ
#include <kernel/sys/types.h>	// for size_t
#include <kernel/tty/uart.h>	// for uart_init_early
#include <libc/string/string.h> // for memset

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

[[noreturn]] static void _sleeper(void *opaque) {
	(void)opaque;
	for (;;) {
		sched_thread_millisleep(750);
		printk("Hello!\n");
	}
}

[[noreturn]] void __kernel_main(void) {
	// 1. zero the BSS section
	memset(__bss, 0, (size_t)(__bss_end - __bss));

	// 2. initialize the serial console
	uart_init_early();

	// 3. intialize the GICv2
	printk("initializing irq...\n");
	irq_init();
	printk("initializing irq... ok\n");

	// 4. initialize the memory manager
	mm_init();

	// 5. create a thread for saying hello to the world
	int64_t tid = sched_thread_start(thread_hello, /* opaque */ 0, /* flags */ 0);
	printk("started hello thread: %d\n", tid);

	// 6. create a thread for saying goodbye to the world
	tid = sched_thread_start(thread_goodbye, /* opaque */ 0, /* flags */ 0);
	printk("started goodbye thread: %d\n", tid);

	// 7. create a thread that prints a message every second
	tid = sched_thread_start(_sleeper, /* opaque */ 0, /* flags */ 0);
	printk("started sleeper thread: %d\n", tid);

	// 8. run the thread scheduler
	printk("starting thread scheduler...\n");
	sched_thread_run();
}
