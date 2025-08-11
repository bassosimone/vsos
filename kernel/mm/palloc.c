// File: kernel/mm/palloc.c
// Purpose: physical-memory-allocator implementation.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/boot/boot.h>	// for __free_ram, __free_ram_end
#include <kernel/core/assert.h> // for KERNEL_ASSERT
#include <kernel/mm/mm.h>	// for arch_mm_phys_addr_t, etc.

#include <string.h> // for memset

// Location where the bump allocator is allocating memory.
static mm_phys_addr_t next_paddr = (mm_phys_addr_t)__free_ram;

mm_phys_addr_t mm_phys_page_alloc_many(size_t n) {
	// 1. make sure the passed size is greater than zero.
	KERNEL_ASSERT(n > 0);

	// 2. save the address of the beginning of the next page.
	mm_phys_addr_t paddr = next_paddr;

	// 3. make sure the multiplication does not overflow uintptr_t.
	KERNEL_ASSERT(n <= UINTPTR_MAX / MM_PAGE_SIZE);

	// 4. compute size to add to the base pointer.
	mm_phys_addr_t size = n * MM_PAGE_SIZE;

	// 5. make sure the sum would not go beyond the end of RAM.
	KERNEL_ASSERT(next_paddr <= (mm_phys_addr_t)__free_ram_end - size);

	// 6. bump the allocator base pointer.
	next_paddr += size;

	// 7. zero the pages using compiler builtins and return.
	memset((void *)paddr, 0, size);
	return paddr;
}
