// File: kernel/mm/palloc.h
// Purpose: physical-memory-allocator interface
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_MM_PALLOC_H
#define KERNEL_MM_PALLOC_H

#include <sys/param.h> // for MM_PAGE_SIZE
#include <sys/types.h> // for size_t

#include <kernel/mm/types.h> // for mm_phys_addr_t, etc.

// Allocates `n` contiguous physical pages and returns the first page address.
//
// Panics in case of failure or if `n` is zero.
mm_phys_addr_t mm_phys_page_alloc_many(size_t n);

#endif // KERNEL_MM_PALLOC_H
