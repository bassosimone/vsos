// File: kernel/boot/boot_arm64.c
// Purpose: Kernel entry point for ARM64
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/boot/boot.h>   // whole subsystem API
#include <kernel/core/assert.h> // for KERNEL_ASSERT
#include <kernel/core/panic.h>  // for panic
#include <kernel/core/printk.h> // for printk
#include <kernel/exec/elf64.h>  // for elf64_parse
#include <kernel/exec/load.h>   // for load_elf64
#include <kernel/init/initrd.h> // for initrd_read_early
#include <kernel/mm/page.h>     // for page_alloc
#include <kernel/mm/vm.h>       // for vm_switch
#include <kernel/sched/sched.h> // whole subsystem API
#include <kernel/trap/trap.h>   // for trap_init_irqs
#include <kernel/tty/uart.h>    // for uart_init_early

#include <sys/param.h> // for HZ
#include <sys/types.h> // for size_t

#include <string.h> // for memset

static void __kernel_zygote(void *opaque) {
	(void)opaque;

	// 1. Initialize the IRQ manager.
	//
	// This will also initialize IRQs for other subsystems.
	//
	// Needs to happen after we have threads.
	trap_init_irqs();

	// 2. load the initial ramdisk
	struct initrd_info rd_info = {.base = 0, .count = 0};
	int rc = initrd_load(&rd_info);
	KERNEL_ASSERT(rc == 0);
	KERNEL_ASSERT(rd_info.base > 0 && rd_info.count > 0);

	// 3. interpret the ramdisk as an ELF binary
	struct elf64_image image = {
	    0,
	};
	rc = elf64_parse(&image, (const void *)rd_info.base, rd_info.count);
	KERNEL_ASSERT(rc == 0);

	// 4. load the ELF binary into memory
	struct load_program program = {
	    0,
	};
	rc = load_elf64(&program, &image);
	KERNEL_ASSERT(rc == 0);

	// 5. the super geronimo thing: return to userspace
	rc = sched_process_start(&program);
	KERNEL_ASSERT(rc >= 0);
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

	// 6. Create the kernel zygote thread.
	//
	// This will enable interrupts and finish bringing the kernel up and running
	int64_t tid = sched_thread_start(__kernel_zygote, /* opaque */ 0, /* flags */ 0);
	printk("created __kernel_zygote thread: %d\n", tid);

	// 7. Run the thread scheduler.
	//
	// Needs to happen before we enable interrupts.
	sched_thread_run();
	panic("unreachable code\n");
}
