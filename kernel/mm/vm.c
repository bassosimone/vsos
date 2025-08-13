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

void __vm_map_explicit(struct vm_root_pt root, page_addr_t paddr, uintptr_t vaddr, vm_map_flags_t flags) {
	// 1. make sure all the addresses are aligned with the page size
	KERNEL_ASSERT(__builtin_is_aligned(root.table, PAGE_SIZE));
	KERNEL_ASSERT(__builtin_is_aligned(paddr, PAGE_SIZE));
	KERNEL_ASSERT(__builtin_is_aligned(vaddr, PAGE_SIZE));

	// 2. let the MD implementation finish the job
	__vm_map_explicit_assume_aligned(root, paddr, vaddr, flags);
}

int64_t vm_map(struct vm_root_pt root,
               page_addr_t start,
               uintptr_t end,
               vm_map_flags_t flags,
               page_addr_t *remapped) {
	KERNEL_ASSERT(mm_align_down(start) == start);

	uint64_t aligned_end = mm_align_up(end);
	printk("  vm_map: table=%llx, start=%llx, end=%llx, aligned_end=%llx, "
	       "size=%lld, aligned_size=%lld, flags=%llx\n",
	       root.table,
	       start,
	       end,
	       aligned_end,
	       end - start,
	       aligned_end - start,
	       flags);

	if (remapped != 0) {
		*remapped = __vm_direct_map(start);
	}

	for (; start < aligned_end; start += MM_PAGE_SIZE) {
		uintptr_t dest = __vm_direct_map(start);
		__vm_map_explicit_assume_aligned(root, start, dest, flags);
		printk("    %llx => %llx\n", start, dest);
		if (remapped != 0) {
			*remapped = start;
		}
	}

	return 0;
}
