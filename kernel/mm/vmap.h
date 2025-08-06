// File: kernel/mm/vmap.h
// Purpose: Virtual-memory-mapper interface.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_MM_VMAP_H
#define KERNEL_MM_VMAP_H

#include <kernel/sys/param.h> // for MM_PAGE_SIZE
#include <kernel/sys/types.h> // for size_t

#include <kernel/mm/types.h> // for mm_phys_addr_t, etc.

// Maps a physical page address to a virtual page address using the given top-level table.
void mm_virt_page_map(mm_phys_addr_t table,
		      mm_phys_addr_t paddr,
		      mm_virt_addr_t vaddr,
		      mm_flags_t flags);

// Internal machine dependent mapping implementation that assumes that
// we have already checked that arguments are correctly aligned
void __mm_virt_page_map_assume_aligned(mm_phys_addr_t table,
				       mm_phys_addr_t paddr,
				       mm_virt_addr_t vaddr,
				       mm_flags_t flags);

#endif // KERNEL_MM_VMAP_H
