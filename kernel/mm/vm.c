// File: kernel/mm/vm.c
// Purpose: Virtual memory manager.
// SPDX-License-Identifier: MIT

#include <kernel/boot/boot.h>   // for __kernel_base
#include <kernel/core/printk.h> // for printk
#include <kernel/mm/page.h>     // for page_alloc
#include <kernel/mm/vm.h>       // for __vm_direct_map
#include <kernel/trap/trap.h>   // for trap_init_mm
#include <kernel/tty/uart.h>    // for uart_init_mm

#include <sys/param.h> // for PAGE_SIZE
#include <sys/types.h> // for uintptr_t

#include <string.h> // for __bzero

// Install the memory map for the kernel memory.
static inline void __vm_map_kernel_memory(struct vm_root_pt root) {
	printk("vm: .text [%llx, %llx) => EXEC\n", __kernel_base, __kernel_end);
	vm_map(root, (uint64_t)__kernel_base, (uint64_t)__kernel_end, VM_MAP_FLAG_EXEC);

	printk("vm: .rodata [%llx, %llx) => 0\n", __rodata_base, __rodata_end);
	vm_map(root, (uint64_t)__rodata_base, (uint64_t)__rodata_end, 0);

	printk("vm: .data [%llx, %llx) => WRITE\n", __data_base, __data_end);
	vm_map(root, (uint64_t)__data_base, (uint64_t)__data_end, VM_MAP_FLAG_WRITE);

	printk("vm: .bss [%llx, %llx) => WRITE\n", __bss_base, __bss_end);
	vm_map(root, (uint64_t)__bss_base, (uint64_t)__bss_end, VM_MAP_FLAG_WRITE);

	printk("vm: .stack [%llx, %llx) => WRITE\n", __stack_bottom, __stack_top);
	vm_map(root, (uint64_t)__stack_bottom, (uint64_t)__stack_top, VM_MAP_FLAG_WRITE);
}

// Requests the devices to install their memory map.
static inline void __vm_map_devices(struct vm_root_pt root) {
	trap_init_mm(root);
	uart_init_mm(root);
}

void vm_switch(void) {
	// 1. create the root table
	printk("vm: switching to virtual addresses... brace yourself\n");
	uintptr_t table = page_must_alloc(PAGE_ALLOC_WAIT);
	__bzero((void *)table, PAGE_SIZE);
	printk("vm: root_table %llx\n", table);
	struct vm_root_pt root = {.table = table};

	// 2. install stuff inside the root table
	__vm_map_kernel_memory(root);
	__vm_map_devices(root);

	// 3. cross our fingers and geronimoooooooooo
	__vm_switch(root);
	printk("vm: we're now running in virtual address space\n");
}

void __vm_map_explicit(struct vm_root_pt root, page_addr_t paddr, uintptr_t vaddr, vm_map_flags_t flags) {
	// 1. make sure all the addresses are aligned with the page size
	KERNEL_ASSERT(__builtin_is_aligned(root.table, PAGE_SIZE));
	KERNEL_ASSERT(__builtin_is_aligned(paddr, PAGE_SIZE));
	KERNEL_ASSERT(__builtin_is_aligned(vaddr, PAGE_SIZE));

	// 2. let the MD implementation finish the job
	__vm_map_explicit_assume_aligned(root, paddr, vaddr, flags);
}

void vm_map(struct vm_root_pt root, page_addr_t start, uintptr_t end, vm_map_flags_t flags) {
	KERNEL_ASSERT(vm_align_down(start) == start);
	end = vm_align_up(end);

	printk("  vm_map: [%llx, %llx) => %lld\n", start, end, flags);
	for (; start < end; start += PAGE_SIZE) {
		printk("    vm_map: [%llx, %llx) => %lld\n", start, start + PAGE_SIZE, flags);
		__vm_map_explicit_assume_aligned(root, start, start, flags);
	}
}
