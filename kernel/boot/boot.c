// File: kernel/boot/boot_arm64.c
// Purpose: Kernel entry point for ARM64
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/boot/boot.h>   // whole subsystem API
#include <kernel/core/panic.h>  // for panic
#include <kernel/core/printk.h> // for printk
#include <kernel/init/switch.h> // for switch_to_userspace
#include <kernel/mm/vm.h>       // for vm_switch
#include <kernel/sched/sched.h> // for sched_thread_start
#include <kernel/trap/trap.h>   // for trap_init_irqs
#include <kernel/tty/uart.h>    // for uart_init_early

#include <sys/types.h> // for size_t

#include <string.h> // for memset

static void __kernel_init_thread(void *opaque) {
	(void)opaque;

	// 1. Initialize the IRQ manager.
	//
	// This will also initialize IRQs for other subsystems.
	//
	// Needs to happen after we have threads.
	trap_init_irqs();

	// 2. Hand off to init subsystem to switch to userspace.
	switch_to_userspace();
}

[[noreturn]] void __kernel_main(void) {
	// 1. Zero the BSS section.
	memset(__bss, 0, (size_t)(__bss_end - __bss));

	// 2. Initialize an early serial console.
	uart_init_early();

	// 3. Initialize the physical page allocator.
	page_init_early();

	// 4. Initialize trap handling structs.
	trap_init_early();

	// 5. Switch to the virtual address space.
	//
	// This is the place that makes everyone very nervous.
	vm_switch();

	// 6. Create the kernel init thread.
	//
	// This will enable interrupts and finish bringing the kernel up and running
	__thread_id_t ketid = 0;
	__status_t rc = sched_thread_start(&ketid, __kernel_init_thread, /* opaque */ 0, /* flags */ 0);
	KERNEL_ASSERT(rc == 0);
	printk("created __kernel_init_thread: %d\n", ketid);

	// 7. Run the thread scheduler.
	//
	// Needs to happen before we enable interrupts.
	sched_thread_run();
	panic("unreachable code\n");
}
