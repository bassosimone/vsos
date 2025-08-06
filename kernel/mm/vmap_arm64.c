// File: kernel/mm/vmap_arm64.c
// Purpose: ARM64 bits of the vmap implementation.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/asm/arm64.h>	// dsb_sy, etc.
#include <kernel/core/assert.h> // KERNEL_ASSERT
#include <kernel/mm/palloc.h>	// mm_phys_page_alloc_many
#include <kernel/mm/vmap.h>	// arch_mm_virt_addr_t, etc.

// We're using identity mapping in this kernel
static inline void *__phys_to_virt(uint64_t paddr) { return (void *)paddr; }

// Physical address mask: page-aligned 4 KiB (bits [11:0] must be 0)
#define ARM64_PTE_ADDR_MASK 0x0000FFFFFFFFF000ULL

// MMU page descriptor flags for 4 KiB pages
#define ARM64_PTE_VALID (1ULL << 0)
#define ARM64_PTE_TABLE (1ULL << 1) /* Used in intermediate levels */
#define ARM64_PTE_AF (1ULL << 10)   /* Access flag — required */
#define ARM64_PTE_UXN (1ULL << 54)
#define ARM64_PTE_PXN (1ULL << 53)

// Access permissions (AP[2:1] shifted to correct positions)
#define ARM64_AP_RW_EL1 (0b00ULL << 6)
#define ARM64_AP_RO_EL1 (0b10ULL << 6)
#define ARM64_AP_RW_EL0 (0b01ULL << 6)
#define ARM64_AP_RO_EL0 (0b11ULL << 6)

// MAIR index (attr index into MAIR_EL1)
#define ARM64_PTE_ATTRINDX(n) (((uint64_t)(n) & 0x7) << 2)

// Indices based on 48-bit VA and 4 KiB pages
#define L1_INDEX(vaddr) (((vaddr) >> 39) & 0x1FF)
#define L2_INDEX(vaddr) (((vaddr) >> 30) & 0x1FF)
#define L3_INDEX(vaddr) (((vaddr) >> 21) & 0x1FF)

// Translate high-level MMU flags to ARM64 PTE descriptor
static uint64_t arm64_make_leaf_pte(mm_phys_addr_t paddr, mm_flags_t flags) {
	uint64_t pte = paddr & ARM64_PTE_ADDR_MASK;

	// Mark as valid, leaf entry
	pte |= ARM64_PTE_VALID | ARM64_PTE_AF;

	// TODO(bassosimone): make memory type configurable (currently attr index 0 = normal memory)
	pte |= ARM64_PTE_ATTRINDX(0);

	// Permissions
	if ((flags & MM_FLAG_WRITE) != 0) {
		pte |= ARM64_AP_RW_EL1;
	} else {
		pte |= ARM64_AP_RO_EL1;
	}

	// TODO(bassosimone): Support user (EL0) mappings in the future
	// For now, we assume kernel-only mappings and we protect ourselves
	KERNEL_ASSERT((flags & MM_FLAG_USER) == 0);

	// Execution permission
	if ((flags & MM_FLAG_EXEC) == 0) {
		pte |= ARM64_PTE_UXN | ARM64_PTE_PXN;
	}

	return pte;
}

static uint64_t arm64_make_table_desc(mm_phys_addr_t paddr) {
	uint64_t desc = paddr & ARM64_PTE_ADDR_MASK;
	desc |= ARM64_PTE_VALID | ARM64_PTE_TABLE;
	return desc;
}

// The caller MUST already have checked that the addresses are all aligned.
void __mm_virt_page_map_assume_aligned(mm_phys_addr_t table,
				       mm_phys_addr_t paddr,
				       mm_virt_addr_t vaddr,
				       mm_flags_t flags) {
	// Step 0: validate assumptions
	KERNEL_ASSERT(MM_PAGE_SIZE == 4096);

	// Step 1: resolve indices
	uint64_t l1_idx = L1_INDEX(vaddr);
	uint64_t l2_idx = L2_INDEX(vaddr);
	uint64_t l3_idx = L3_INDEX(vaddr);

	// TODO(bassosimone): refactor duplicated code into a single
	// function named ensure_table_entry, maybe.

	// Step 2: walk L1
	uint64_t *l1 = (uint64_t *)__phys_to_virt(table);
	if ((l1[l1_idx] & ARM64_PTE_VALID) == 0) {
		mm_phys_addr_t l2_phys = mm_phys_page_alloc_many(1);
		__builtin_memset(__phys_to_virt(l2_phys), 0, MM_PAGE_SIZE);
		l1[l1_idx] = arm64_make_table_desc(l2_phys);
		dsb_sy(); // ensure table write is visible
	}

	// Step 3: walk L2
	uint64_t *l2 = (uint64_t *)__phys_to_virt(l1[l1_idx] & ARM64_PTE_ADDR_MASK);
	if ((l2[l2_idx] & ARM64_PTE_VALID) == 0) {
		mm_phys_addr_t l3_phys = mm_phys_page_alloc_many(1);
		__builtin_memset(__phys_to_virt(l3_phys), 0, MM_PAGE_SIZE);
		l2[l2_idx] = arm64_make_table_desc(l3_phys);
		dsb_sy(); // ensure table write is visible
	}

	// Step 4: walk L3 (leaf)
	uint64_t *l3 = (uint64_t *)__phys_to_virt(l2[l2_idx] & ARM64_PTE_ADDR_MASK);
	uint64_t pte = arm64_make_leaf_pte(paddr, flags);
	l3[l3_idx] = pte;
	dsb_sy(); // ensure page mapping is visible

	// TODO(bassosimone): as soon as we start context switching this
	// function will break because we are not flushing the TLB.
}
