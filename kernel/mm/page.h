// File: kernel/mm/page.h
// Purpose: Physical pages allocator.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_MM_PAGE_H
#define KERNEL_MM_PAGE_H

#include <kernel/core/assert.h> // for kernel_assert

#include <sys/types.h> // for uintptr_t

// Physical address of a memory page.
typedef uintptr_t page_addr_t;

// It is okay to wait for free pages to become available.
#define PAGE_ALLOC_WAIT (1 << 0)

// It is okay to yield when waiting.
#define PAGE_ALLOC_YIELD (1 << 1)

// Early initialization of the page allocator.
//
// Called early by the boot subsystem.
void page_init_early(void);

// Allocate a single memory page.
//
// Returns 0 on success and `-ENOMEM` or `-EAGAIN` on failure.
//
// The addr is set to zero in case of error.
int64_t page_alloc(page_addr_t *addr, uint64_t flags);

// Convenience function for allocations that MUST always succeed.
static inline page_addr_t page_must_alloc(uint64_t flags) {
	page_addr_t addr = 0;
	KERNEL_ASSERT(page_alloc(&addr, flags) == 0);
	return addr;
}

// Free a memory page given its address.
//
// Panics when the address is not aligned or the page is not allocated.
void page_free(page_addr_t addr);

// Prints the bitmask using printk.
void page_debug_printk(void);

#endif // KERNEL_MM_PAGE_H
