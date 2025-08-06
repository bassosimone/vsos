// File: kernel/mm/typesgeneric.h
// Purpose: Generic memory manager types.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_MM_TYPES_H
#define KERNEL_MM_TYPES_H

#include <kernel/sys/types.h> // for uintptr_t, etc.

// Portable definitions of page mapping flags.
#define MM_FLAG_PRESENT (1 << 0)
#define MM_FLAG_WRITE (1 << 1)
#define MM_FLAG_EXEC (1 << 2)
#define MM_FLAG_USER (1 << 3)

// Physical and virtual address definitions on amd64.
typedef uintptr_t mm_phys_addr_t;
typedef uintptr_t mm_virt_addr_t;

// Integer type containing the page mapping flags.
typedef uint32_t mm_flags_t;

#endif // KERNEL_MM_TYPES_H
