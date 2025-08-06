// File: kernel/mm/vmap.c
// Purpose: Virtual-memory-mapper implementation.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/core/assert.h> // for KERNEL_ASSERT

#include <kernel/mm/vmap.h> // for arch_mm_phys_addr_t, etc.

void mm_virt_page_map(mm_phys_addr_t table, mm_phys_addr_t paddr, mm_virt_addr_t vaddr, mm_flags_t flags) {
	// 1. make sure all the addresses are aligned with the page size
	KERNEL_ASSERT(__builtin_is_aligned(table, MM_PAGE_SIZE));
	KERNEL_ASSERT(__builtin_is_aligned(paddr, MM_PAGE_SIZE));
	KERNEL_ASSERT(__builtin_is_aligned(vaddr, MM_PAGE_SIZE));

	// 2. let the MD implementation finish the job
	__mm_virt_page_map_assume_aligned(table, paddr, vaddr, flags);
}
