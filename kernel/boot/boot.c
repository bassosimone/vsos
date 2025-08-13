// File: kernel/boot/boot_arm64.c
// Purpose: Kernel entry point for ARM64
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/boot/boot.h>   // whole subsystem API
#include <kernel/core/assert.h> // for KERNEL_ASSERT
#include <kernel/core/panic.h>  // for panic
#include <kernel/core/printk.h> // for printk
#include <kernel/mm/page.h>     // for page_alloc
#include <kernel/mm/vm.h>       // for vm_switch
#include <kernel/sched/sched.h> // whole subsystem API
#include <kernel/trap/trap.h>   // for trap_init_irqs
#include <kernel/tty/uart.h>    // for uart_init_early

#include <sys/param.h> // for HZ
#include <sys/types.h> // for size_t

#include <string.h> // for memset

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

static void _sleeper(void *opaque) {
	(void)opaque;
	for (size_t idx = 0; idx < 4; idx++) {
		sched_thread_millisleep(750);
		printk("Hello!\n");
	}
}

// Very minimal mechanism to ensure we echo the input.
[[noreturn]] static void _echo_thread(void *opaque) {
	(void)opaque;
	for (;;) {
		char ch = 0;
		ssize_t n = uart_recv(&ch, 1, /* flags */ 0);
		if (n != 1) {
			continue;
		}
		if (ch == '\r') {
			ch = '\n';
		}
		(void)uart_send(&ch, 1, /* flags */ 0);
	}
}

static void __kernel_zygote(void *opaque) {
	(void)opaque;

	// 1. Initialize the IRQ manager.
	//
	// This will also initialize IRQs for other subsystems.
	//
	// Needs to happen after we have threads.
	trap_init_irqs();

	// 2. create a thread for saying hello to the world
	int64_t tid = sched_thread_start(thread_hello, /* opaque */ 0, /* flags */ 0);
	printk("started hello thread: %d\n", tid);

	// 3. create a thread for saying goodbye to the world
	tid = sched_thread_start(thread_goodbye, /* opaque */ 0, /* flags */ 0);
	printk("started goodbye thread: %d\n", tid);

	// 4. create a thread that prints a message every second
	tid = sched_thread_start(_sleeper, /* opaque */ 0, /* flags */ 0);
	printk("started sleeper thread: %d\n", tid);

	// 5. create a thread that echoes what we write.
	tid = sched_thread_start(_echo_thread, /* opaque */ 0, /* flags */ 0);
	printk("started echo thread: %d\n", tid);
}

[[noreturn]] void __kernel_main(void) {
	// 1. Zero the BSS section.
	memset(__bss, 0, (size_t)(__bss_end - __bss));

	// 2. Initialize an early serial console.
	uart_init_early();

	// 3. Initialize the physical page allocator.
	page_init_early();

	// 4. Switch to the virtual address space.
	//
	// This is the place that makes everyone very nervous.
	vm_switch();

	// 5. Create the kernel zygote thread.
	//
	// This will enable interrupts and finish bringing the kernel up and running
	int64_t tid = sched_thread_start(__kernel_zygote, /* opaque */ 0, /* flags */ 0);
	printk("created __kernel_zygote thread: %d\n", tid);

	// 6. Run the thread scheduler.
	//
	// Needs to happen before we enable interrupts.
	sched_thread_run();
	panic("unreachable code\n");
}
