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
#define mm_virt_page_map __vm_map_explicit

// Internal machine dependent mapping implementation that assumes that
// we have already checked that arguments are correctly aligned
#define __mm_virt_page_map_assume_aligned __vm_map_explicit_assume_aligned

// Maps maps the pages between the given start and end using identity mapping.
void mm_map_identity(struct vm_root_pt root, uintptr_t start, uintptr_t end, mm_flags_t flags);

#endif // KERNEL_MM_MM_H
