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

void vm_map_kernel_memory(struct vm_root_pt root) {
	printk("vm: <0x%llx> .text [%llx, %llx) => EXEC\n", root.table, __kernel_base, __kernel_end);
	vm_map_range_identity(root, (uint64_t)__kernel_base, (uint64_t)__kernel_end, VM_MAP_FLAG_EXEC);

	printk("vm: <0x%llx> .rodata [%llx, %llx) => 0\n", root.table, __rodata_base, __rodata_end);
	vm_map_range_identity(root, (uint64_t)__rodata_base, (uint64_t)__rodata_end, 0);

	printk("vm: <0x%llx> .data [%llx, %llx) => WRITE\n", root.table, __data_base, __data_end);
	vm_map_range_identity(root, (uint64_t)__data_base, (uint64_t)__data_end, VM_MAP_FLAG_WRITE);

	printk("vm: <0x%llx> .bss [%llx, %llx) => WRITE\n", root.table, __bss_base, __bss_end);
	vm_map_range_identity(root, (uint64_t)__bss_base, (uint64_t)__bss_end, VM_MAP_FLAG_WRITE);

	printk("vm: <0x%llx> .stack [%llx, %llx) => WRITE\n", root.table, __stack_bottom, __stack_top);
	vm_map_range_identity(root, (uint64_t)__stack_bottom, (uint64_t)__stack_top, VM_MAP_FLAG_WRITE);

	printk("vm: <0x%llx> __free_ram [%llx, %llx) => WRITE\n", root.table, __free_ram_start, __free_ram_end);
	vm_map_range_identity(root, (uint64_t)__free_ram_start, (uint64_t)__free_ram_end, VM_MAP_FLAG_WRITE);
}

void vm_map_devices(struct vm_root_pt root) {
	trap_init_mm(root);
	uart_init_mm(root);
}

uintptr_t __vm_kernel_root_pt;

struct vm_root_pt vm_kernel_root_pt(void) {
	KERNEL_ASSERT(__vm_kernel_root_pt != 0);
	return (struct vm_root_pt){.table = __vm_kernel_root_pt};
}

void vm_switch(void) {
	// 1. create the root table
	KERNEL_ASSERT(__vm_kernel_root_pt == 0);
	printk("vm: switching to virtual addresses... brace yourself\n");
	__vm_kernel_root_pt = page_must_alloc(PAGE_ALLOC_WAIT);
	printk("vm: root_table %llx\n", __vm_kernel_root_pt);
	struct vm_root_pt __root = vm_kernel_root_pt();

	// 2. install stuff inside the root table
	vm_map_kernel_memory(__root);
	vm_map_devices(__root);

	// 3. cross our fingers and geronimoooooooooo
	__vm_switch(__root);
	printk("vm: we're now running in virtual address space\n");
}

void vm_map_explicit(struct vm_root_pt root, page_addr_t paddr, uintptr_t vaddr, vm_map_flags_t flags) {
	// 1. make sure all the addresses are aligned with the page size
	KERNEL_ASSERT(__builtin_is_aligned(root.table, PAGE_SIZE));
	KERNEL_ASSERT(__builtin_is_aligned(paddr, PAGE_SIZE));
	KERNEL_ASSERT(__builtin_is_aligned(vaddr, PAGE_SIZE));

	// 2. if needed print what we're doing
	if ((flags & VM_MAP_FLAG_DEBUG) != 0) {
		printk("    vm_map: [%llx, %llx) <-> [%llx, %llx) => %lld\n",
		       paddr,
		       paddr + PAGE_SIZE,
		       vaddr,
		       vaddr + PAGE_SIZE,
		       flags);
	}

	// 2. let the MD implementation finish the job
	__vm_map_explicit_assume_aligned(root, paddr, vaddr, flags);
}

void vm_map_range_identity(struct vm_root_pt root, page_addr_t start, uintptr_t end, vm_map_flags_t flags) {
	KERNEL_ASSERT(vm_align_down(start) == start);
	end = vm_align_up(end);

	// We print the high-level range mapping because it's just one line per range
	printk("  vm_map: [%llx, %llx) => %lld\n", start, end, flags);
	for (; start < end; start += PAGE_SIZE) {
		vm_map_identity(root, start, flags);
	}
}
