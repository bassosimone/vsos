// File: kernel/mm/vmap.c
// Purpose: Virtual-memory-mapper implementation.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/core/assert.h> // for KERNEL_ASSERT
#include <kernel/core/printk.h> // for printk

#include <kernel/mm/mm.h> // for mm_map_identity, etc.

void mm_map_identity(struct vm_root_pt root, page_addr_t start, uintptr_t end, vm_map_flags_t flags) {
	uint64_t aligned_start = mm_align_down(start);
	uint64_t aligned_end = mm_align_up(end);
	printk("  mm_map_identity: table=%llx, start=%llx, aligned_start=%llx, end=%llx, aligned_end=%llx, "
	       "size=%lld, aligned_size=%lld, flags=%llx\n",
	       root.table,
	       start,
	       aligned_start,
	       end,
	       aligned_end,
	       end - start,
	       aligned_end - aligned_start,
	       flags);
	for (; aligned_start < aligned_end; aligned_start += MM_PAGE_SIZE) {
		printk("    %llx => %llx\n", aligned_start, aligned_start);
		__vm_map_explicit_assume_aligned(root, aligned_start, aligned_start, flags);
	}
}
