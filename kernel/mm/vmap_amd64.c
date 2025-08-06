// File: kernel/mm/vmap_amd64.c
// Purpose: AMD64 bits of the vmap implementation.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/core/assert.h> // for KERNEL_ASSERT
#include <kernel/mm/palloc.h>	// for mm_phys_page_alloc_many
#include <kernel/mm/vmap.h>	// for arch_mm_phys_addr_t, etc.

// We're using identity mapping in this kernel
static inline void *__phys_to_virt(uint64_t paddr) { return (void *)paddr; }

/*
 < 63-48 | 47-39  | 38-30  | 29-21  | 20-12  | 11-0  >
 [unused | PML4   | PDPT   | PD     | PT     | offset]
*/
#define AMD64_PTE_ADDR_MASK 0x000FFFFFFFFFF000ULL

// Shifts used by the AMD64 architecture
#define AMD64_SHIFT_PRESENT 1
#define AMD64_SHIFT_WRITE 2
#define AMD64_SHIFT_USER 3
#define AMD64_SHIFT_NX 63 /* careful: opposite logic */

// Flags used by the AMD64 architecture
#define AMD64_FLAG_PRESENT (1ULL << AMD64_SHIFT_PRESENT)

// The caller MUST already have checked that the addresses are all aligned.
void __mm_virt_page_map_assume_aligned(mm_phys_addr_t table,
				       mm_phys_addr_t paddr,
				       mm_virt_addr_t vaddr,
				       mm_flags_t flags) {
	// Step 0: validate algorithmic assumptions
	KERNEL_ASSERT(MM_PAGE_SIZE == 4096);

	// Step 1: resolve indices
	uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
	uint64_t pdpt_idx = (vaddr >> 30) & 0x1FF;
	uint64_t pd_idx = (vaddr >> 21) & 0x1FF;
	uint64_t pt_idx = (vaddr >> 12) & 0x1FF;

	// TODO(bassosimone): refactor duplicated code into a single
	// function named ensure_table_entry, maybe.

	// Step 2: determine the flags to use for intermediate PT entries
	uint64_t intermediate_flags = 0;
	intermediate_flags |= (1ULL << AMD64_SHIFT_PRESENT);
	intermediate_flags |= (1ULL << AMD64_SHIFT_WRITE);
	if ((flags & MM_FLAG_USER) != 0) {
		intermediate_flags |= (1ULL << AMD64_SHIFT_USER);
	}
	intermediate_flags |= (1ULL << AMD64_SHIFT_NX);

	// Step 3: resolve PML4 (page map level 4)
	uint64_t *pml4 = (uint64_t *)__phys_to_virt(table);

	if ((pml4[pml4_idx] & AMD64_FLAG_PRESENT) == 0) {
		mm_phys_addr_t pdpt_phys = mm_phys_page_alloc_many(1);
		__builtin_memset(__phys_to_virt(pdpt_phys), 0, MM_PAGE_SIZE);
		pml4[pml4_idx] = pdpt_phys | intermediate_flags;
	}

	// Step 4: resolve PDPT (page directory pointer table)
	uint64_t *pdpt = (uint64_t *)__phys_to_virt(pml4[pml4_idx] & AMD64_PTE_ADDR_MASK);

	if ((pdpt[pdpt_idx] & AMD64_FLAG_PRESENT) == 0) {
		mm_phys_addr_t pd_phys = mm_phys_page_alloc_many(1);
		__builtin_memset(__phys_to_virt(pd_phys), 0, MM_PAGE_SIZE);
		pdpt[pdpt_idx] = pd_phys | intermediate_flags;
	}

	// Step 5: resolve PD (page directory)
	uint64_t *pd = (uint64_t *)__phys_to_virt(pdpt[pdpt_idx] & AMD64_PTE_ADDR_MASK);

	if ((pd[pd_idx] & AMD64_FLAG_PRESENT) == 0) {
		mm_phys_addr_t pt_phys = mm_phys_page_alloc_many(1);
		__builtin_memset(__phys_to_virt(pt_phys), 0, MM_PAGE_SIZE);
		pd[pd_idx] = pt_phys | intermediate_flags;
	}

	// Step 6: resolve PT (page table)
	uint64_t *pt = (uint64_t *)__phys_to_virt(pd[pd_idx] & AMD64_PTE_ADDR_MASK);

	// Step 7: create PTE
	uint64_t pte = (paddr & AMD64_PTE_ADDR_MASK);

	if ((flags & MM_FLAG_PRESENT) != 0) {
		pte |= (1ULL << AMD64_SHIFT_PRESENT);
	}
	if ((flags & MM_FLAG_WRITE) != 0) {
		pte |= (1ULL << AMD64_SHIFT_WRITE);
	}
	if ((flags & MM_FLAG_USER) != 0) {
		pte |= (1ULL << AMD64_SHIFT_USER);
	}
	if ((flags & MM_FLAG_EXEC) == 0) { // opposite logic
		pte |= (1ULL << AMD64_SHIFT_NX);
	}

	pt[pt_idx] = pte;
}
