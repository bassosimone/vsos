// File: kernel/mm/vmap.c
// Purpose: Virtual-memory-mapper implementation.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/core/assert.h> // for KERNEL_ASSERT
#include <kernel/core/printk.h> // for printk

#include <kernel/mm/types.h> // for arch_mm_phys_addr_t, etc.
#include <kernel/mm/vmap.h>  // for mm_map_identity, etc.

void mm_virt_page_map(mm_phys_addr_t table, mm_phys_addr_t paddr, mm_virt_addr_t vaddr, mm_flags_t flags) {
	// 1. make sure all the addresses are aligned with the page size
	KERNEL_ASSERT(__builtin_is_aligned(table, MM_PAGE_SIZE));
	KERNEL_ASSERT(__builtin_is_aligned(paddr, MM_PAGE_SIZE));
	KERNEL_ASSERT(__builtin_is_aligned(vaddr, MM_PAGE_SIZE));

	// 2. let the MD implementation finish the job
	__mm_virt_page_map_assume_aligned(table, paddr, vaddr, flags);
}

void mm_map_identity(mm_phys_addr_t table, mm_phys_addr_t start, mm_phys_addr_t end, mm_flags_t flags) {
	uint64_t aligned_start = mm_align_down(start);
	uint64_t aligned_end = mm_align_up(end);
	printk("  mm_map_identity: table=%llx, start=%llx, aligned_start=%llx, end=%llx, aligned_end=%llx, "
	       "size=%lld, aligned_size=%lld, flags=%llx\n",
	       table,
	       start,
	       aligned_start,
	       end,
	       aligned_end,
	       end - start,
	       aligned_end - aligned_start,
	       flags);
	for (; aligned_start < aligned_end; aligned_start += MM_PAGE_SIZE) {
		printk("    %llx => %llx\n", aligned_start, aligned_start);
		__mm_virt_page_map_assume_aligned(table, aligned_start, aligned_start, flags);
	}
}
