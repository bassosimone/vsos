// File: kernel/mm/vmap.h
// Purpose: Virtual-memory-mapper interface.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_MM_VMAP_H
#define KERNEL_MM_VMAP_H

#include <kernel/mm/types.h>  // for mm_phys_addr_t, etc.
#include <kernel/sys/param.h> // for MM_PAGE_SIZE
#include <kernel/sys/types.h> // for size_t

// Maps a physical page address to a virtual page address using the given top-level table.
void mm_virt_page_map(mm_phys_addr_t table, mm_phys_addr_t paddr, mm_virt_addr_t vaddr, mm_flags_t flags);

// Internal machine dependent mapping implementation that assumes that
// we have already checked that arguments are correctly aligned
void __mm_virt_page_map_assume_aligned(mm_phys_addr_t table,
				       mm_phys_addr_t paddr,
				       mm_virt_addr_t vaddr,
				       mm_flags_t flags);

// Maps maps the pages between the given start and end using identity mapping.
void mm_map_identity(mm_phys_addr_t table, mm_phys_addr_t start, mm_phys_addr_t end, mm_flags_t flags);

// More general memory mapper that does not depend on the kernel base page table.
void mmap_identity(mm_phys_addr_t start, mm_phys_addr_t end, mm_flags_t flags);

// Initialize the memory manager
void mm_init(void);

#endif // KERNEL_MM_VMAP_H
