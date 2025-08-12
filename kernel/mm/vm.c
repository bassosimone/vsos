// File: kernel/mm/vm.c
// Purpose: Virtual memory manager.
// SPDX-License-Identifier: MIT

#include <kernel/boot/boot.h>   // for __kernel_base
#include <kernel/core/printk.h> // for printk
#include <kernel/mm/mm.h>       // for mm_map_identity
#include <kernel/mm/page.h>     // for page_alloc
#include <kernel/mm/vm.h>       // for __vm_direct_map
#include <kernel/trap/trap.h>   // for trap_init_mm
#include <kernel/tty/uart.h>    // for uart_init_mm

#include <sys/types.h> // for uintptr_t

uintptr_t __vm_direct_map(uintptr_t phys_addr) {
	// TODO(bassosimone): implement direct mapping
	return phys_addr;
}

// Install the memory map for the kernel memory.
static inline void __vm_map_kernel_memory(struct vm_root_pt root) {
	printk("vm: mapping __kernel_base %llx -> __kernel_end %llx\n", __kernel_base, __kernel_end);
	mm_map_identity(root, (uint64_t)__kernel_base, (uint64_t)__kernel_end, MM_FLAG_EXEC);

	printk("vm: mapping __rodata_base %llx -> __rodata_end %llx\n", __rodata_base, __rodata_end);
	mm_map_identity(root, (uint64_t)__rodata_base, (uint64_t)__rodata_end, 0);

	printk("vm: mapping __data_base %llx -> __data_end %llx\n", __data_base, __data_end);
	mm_map_identity(root, (uint64_t)__data_base, (uint64_t)__data_end, MM_FLAG_WRITE);

	printk("vm: mapping __bss_base %llx -> __bss_end %llx\n", __bss_base, __bss_end);
	mm_map_identity(root, (uint64_t)__bss_base, (uint64_t)__bss_end, MM_FLAG_WRITE);

	printk("vm: mapping __stack_bottom %llx -> __stack_top %llx\n", __stack_bottom, __stack_top);
	mm_map_identity(root, (uint64_t)__stack_bottom, (uint64_t)__stack_top, MM_FLAG_WRITE);
}

// Requests the devices to install their memory map.
static inline void __vm_map_devices(struct vm_root_pt root) {
	trap_init_mm(root);
	uart_init_mm(root);
}

void vm_switch(void) {
	// 1. create the root table
	uintptr_t table = page_must_alloc(PAGE_ALLOC_WAIT);
	printk("vm: root_table %llx\n", table);
	struct vm_root_pt root = {.table = table};

	// 2. install stuff inside the root table
	__vm_map_kernel_memory(root);
	__vm_map_devices(root);

	// 3. cross our fingers and geronimoooooooooo
	__vm_switch_to_virtual(root);
}
