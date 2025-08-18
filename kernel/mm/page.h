// File: kernel/mm/page.h
// Purpose: Physical pages allocator.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_MM_PAGE_H
#define KERNEL_MM_PAGE_H

#include <kernel/core/assert.h> // for kernel_assert

#include <sys/param.h> // for PAGE_SIZE
#include <sys/types.h> // for uintptr_t

// Physical address of a memory page.
typedef uintptr_t page_addr_t;

// How many bits we need to shift to get a page ID from an address.
//
// We statically check this is consistent inside mm/page.c.
#define PAGE_SHIFT 12

// Mask where all page bits are zero and the offset inside the page is nonzero.
//
// We statically check this is consistent inside mm/page.c.
#define PAGE_OFFSET_MASK (PAGE_SIZE - 1)

// Returns true if the given address is page aligned.
static inline bool page_aligned(uintptr_t addr) {
	return (addr & PAGE_OFFSET_MASK) == 0;
}

// It is okay to wait for free pages to become available.
#define PAGE_ALLOC_WAIT (1 << 0)

// It is okay to yield when waiting.
#define PAGE_ALLOC_YIELD (1 << 1)

// Print details about what we are actually allocating.
#define PAGE_ALLOC_DEBUG (1 << 2)

// Early initialization of the page allocator.
//
// Called early by the boot subsystem.
void page_init_early(void);

// Allocate a single memory page.
//
// The returned memory page is *physical*. However, the kernel maps the
// whole RAM, therefore, for the kernel it is also virtual.
//
// The returned memory page *content* is zeroed. This is possible
// because the kernel identity maps the RAM.
//
// Returns 0 on success and `-ENOMEM` or `-EAGAIN` on failure.
//
// The addr is set to zero in case of error.
__status_t page_alloc(page_addr_t *addr, __flags32_t flags);

// Convenience function for allocations that MUST always succeed.
static inline page_addr_t page_must_alloc(__flags32_t flags) {
	page_addr_t addr = 0;
	KERNEL_ASSERT(page_alloc(&addr, flags) == 0);
	return addr;
}

// Free a memory page given its address.
//
// The given address is *physical*. However, we use identity mapping and the
// kernel maps the whole available RAM, so it is also virtual.
//
// The flags allow you to pass PAGE_ALLOC_DEBUG for debug printing.
//
// Panics when the address is not aligned or the page is not allocated.
void page_free(page_addr_t addr, __flags32_t flags);

// Prints the bitmask using printk.
void page_debug_printk(void);

#endif // KERNEL_MM_PAGE_H
