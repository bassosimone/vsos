// File: kernel/mm/mm.h
// Purpose: Memory management subsystem
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_MM_MM_H
#define KERNEL_MM_MM_H

#include <kernel/mm/page.h> // for page_addr_t
#include <kernel/mm/vm.h>   // for VM_MAP_FLAG_*

#include <sys/param.h> // for MM_PAGE_SIZE
#include <sys/types.h> // for uintptr_t, etc.

// Backward compatible definitions
#define MM_FLAG_WRITE VM_MAP_FLAG_WRITE
#define MM_FLAG_EXEC VM_MAP_FLAG_EXEC
#define MM_FLAG_USER VM_MAP_FLAG_USER
#define MM_FLAG_DEVICE VM_MAP_FLAG_DEVICE
#define mm_flags_t vm_map_flags_t
#define mm_align_down vm_align_down
#define mm_align_up vm_align_up

// Maps a physical page address to a virtual page address using the given top-level table.
void mm_virt_page_map(uintptr_t table, page_addr_t paddr, uintptr_t vaddr, mm_flags_t flags);

// Internal machine dependent mapping implementation that assumes that
// we have already checked that arguments are correctly aligned
void __mm_virt_page_map_assume_aligned(uintptr_t table, page_addr_t paddr, uintptr_t vaddr, mm_flags_t flags);

// Maps maps the pages between the given start and end using identity mapping.
void mm_map_identity(uintptr_t table, uintptr_t start, uintptr_t end, mm_flags_t flags);

#endif // KERNEL_MM_MM_H
