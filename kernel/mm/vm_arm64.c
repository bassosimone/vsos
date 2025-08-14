// File: kernel/mm/vm_arm64.c
// Purpose: ARM64 bits of the VM implementation.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/asm/arm64.h>   // for dsb_sy, etc.
#include <kernel/boot/boot.h>   // for __kernel_base
#include <kernel/core/assert.h> // for KERNEL_ASSERT
#include <kernel/core/printk.h> // for printk
#include <kernel/mm/page.h>     // for page_alloc
#include <kernel/mm/vm.h>       // for __vm_map_kernel_memory

// Physical address mask: page-aligned 4 KiB (bits [11:0] must be 0)
#define ARM64_PTE_ADDR_MASK 0x0000FFFFFFFFF000ULL

// MMU page descriptor flags for 4 KiB pages
#define ARM64_PTE_VALID (1ULL << 0)
#define ARM64_PTE_TABLE (1ULL << 1)
#define ARM64_PTE_AF (1ULL << 10) /* Access flag — required */
#define ARM64_PTE_nG (1ULL << 11)
#define ARM64_PTE_UXN (1ULL << 54)
#define ARM64_PTE_PXN (1ULL << 53)

// Access permissions (AP[2:1] shifted to correct positions)
#define ARM64_AP_RW_EL1 (0b00ULL << 6)
#define ARM64_AP_RO_EL1 (0b10ULL << 6)
#define ARM64_AP_RW_EL0 (0b01ULL << 6)
#define ARM64_AP_RO_EL0 (0b11ULL << 6)

// MAIR index (attr index into MAIR_EL1)
#define ARM64_PTE_ATTRINDX(n) (((uint64_t)(n) & 0x7) << 2)

// Indices based on 39-bit VA and 4 KiB pages
#define L1_INDEX(vaddr) (((vaddr) >> 30) & 0x1FF)
#define L2_INDEX(vaddr) (((vaddr) >> 21) & 0x1FF)
#define L3_INDEX(vaddr) (((vaddr) >> 12) & 0x1FF)

// Memory sharability bits
#define ARM64_PTE_SH_NON (0ULL << 8)
#define ARM64_PTE_SH_OUTER (2ULL << 8)
#define ARM64_PTE_SH_INNER (3ULL << 8)

// MAIR: idx0 = Normal WBWA, idx1 = Device-nGnRE
#define MAIR_ATTR_NORMAL_WBWA 0xFF
#define MAIR_ATTR_DEVICE_nGnRE 0x04

// Creates a leaf page table entry.
static uint64_t make_leaf_pte(uintptr_t paddr, vm_map_flags_t flags) {
	uint64_t pte = paddr & ARM64_PTE_ADDR_MASK;

	// Valid leaf
	pte |= ARM64_PTE_VALID | ARM64_PTE_TABLE | ARM64_PTE_AF;

	// Memory type + shareability
	if ((flags & VM_MAP_FLAG_DEVICE) != 0) {
		// Device-nGnRE is common for MMIO (AttrIdx=1), SH is ignored for Device but OUTER is fine
		pte |= ARM64_PTE_ATTRINDX(1) | ARM64_PTE_SH_OUTER;
		// Device memory MUST NOT be executable by kernel or user
		pte |= ARM64_PTE_UXN | ARM64_PTE_PXN;
	} else {
		// Normal WB WA cacheable (AttrIdx=0), Inner-shareable for coherency
		pte |= ARM64_PTE_ATTRINDX(0) | ARM64_PTE_SH_INNER;
	}

	const bool is_user = (flags & VM_MAP_FLAG_USER) != 0;
	const bool can_write = (flags & VM_MAP_FLAG_WRITE) != 0;
	const bool can_exec = (flags & VM_MAP_FLAG_EXEC) != 0;

	// AP: choose exactly one encoding
	if (is_user) {
		pte |= can_write ? ARM64_AP_RW_EL0 : ARM64_AP_RO_EL0;
	} else {
		pte |= can_write ? ARM64_AP_RW_EL1 : ARM64_AP_RO_EL1;
	}

	// XN bits:
	// - User pages should never be executable at EL1
	// - Kernel pages should never be executable at EL0
	if (is_user) {
		pte |= ARM64_PTE_PXN;
		if (!can_exec) {
			pte |= ARM64_PTE_UXN;
		}
	} else {
		pte |= ARM64_PTE_UXN;
		if (!can_exec) {
			pte |= ARM64_PTE_PXN;
		}
	}

	// W^X warnings
	if (can_write && can_exec) {
		printk("vm: W|X is discouraged for %sspace\n", is_user ? "user" : "kernel");
	}

	// Mark user pages as non-global to protect
	// the memory assigned to each user
	if (is_user) {
		pte |= ARM64_PTE_nG;
	}

	return pte;
}

// Creates an intermediate table descriptor.
static uint64_t make_intermediate_table_desc(uintptr_t paddr) {
	uint64_t desc = paddr & ARM64_PTE_ADDR_MASK;
	desc |= ARM64_PTE_VALID | ARM64_PTE_TABLE;
	return desc;
}

// The caller MUST already have checked that the addresses are all aligned.
void __vm_map_explicit_assume_aligned(struct vm_root_pt root,
                                      page_addr_t paddr,
                                      uintptr_t vaddr,
                                      vm_map_flags_t flags) {
	// 1. validate assumptions
	KERNEL_ASSERT(PAGE_SIZE == 4096);

	// 2. see whether we need to debug page allocations
	uint64_t palloc_flags = PAGE_ALLOC_WAIT;
	if ((flags & VM_MAP_FLAG_DEBUG) != 0) {
		palloc_flags |= PAGE_ALLOC_DEBUG;
	}

	// 3. resolve indices
	uint64_t l1_idx = L1_INDEX(vaddr);
	if ((flags & VM_MAP_FLAG_DEBUG) != 0) {
		printk("      L1_INDEX(%llx) = %llx\n", vaddr, l1_idx);
	}

	uint64_t l2_idx = L2_INDEX(vaddr);
	if ((flags & VM_MAP_FLAG_DEBUG) != 0) {
		printk("      L2_INDEX(%llx) = %llx\n", vaddr, l2_idx);
	}

	uint64_t l3_idx = L3_INDEX(vaddr);
	if ((flags & VM_MAP_FLAG_DEBUG) != 0) {
		printk("      L3_INDEX(%llx) = %llx\n", vaddr, l3_idx);
	}

	// 4. walk L1
	uint64_t *l1_phys = (uint64_t *)root.table;
	if ((flags & VM_MAP_FLAG_DEBUG) != 0) {
		printk("      L1_PHYS = %llx\n", l1_phys);
	}

	uint64_t *l1_virt = l1_phys; // direct mapping
	if ((flags & VM_MAP_FLAG_DEBUG) != 0) {
		printk("      L1_VIRT = %llx\n", l1_virt);
	}

	if ((l1_virt[l1_idx] & ARM64_PTE_VALID) == 0) {
		uintptr_t l2_phys = page_must_alloc(palloc_flags);
		l1_virt[l1_idx] = make_intermediate_table_desc(l2_phys);
		dsb_ishst(); // ensure visibility
	}
	if ((flags & VM_MAP_FLAG_DEBUG) != 0) {
		printk("      L1_VIRT[L1_INDEX] = %llx\n", l1_virt[l1_idx]);
	}

	// 5. walk L2
	uint64_t *l2_phys = (uint64_t *)(l1_virt[l1_idx] & ARM64_PTE_ADDR_MASK);
	if ((flags & VM_MAP_FLAG_DEBUG) != 0) {
		printk("      L2_PHYS = %llx\n", l2_phys);
	}

	uint64_t *l2_virt = l2_phys; // direct mapping

	if ((l2_virt[l2_idx] & ARM64_PTE_VALID) == 0) {
		uintptr_t l3_phys = page_must_alloc(palloc_flags);
		l2_virt[l2_idx] = make_intermediate_table_desc(l3_phys);
		dsb_ishst(); // ensure visibility
	}
	if ((flags & VM_MAP_FLAG_DEBUG) != 0) {
		printk("      L2_VIRT[L2_INDEX] = %llx\n", l2_virt[l2_idx]);
	}

	// 6. try to insert the L3 leaf
	uint64_t *l3_phys = (uint64_t *)(l2_virt[l2_idx] & ARM64_PTE_ADDR_MASK);

	uint64_t *l3_virt = l3_phys; // direct mapping

	KERNEL_ASSERT((l3_virt[l3_idx] & ARM64_PTE_VALID) == 0);

	l3_virt[l3_idx] = make_leaf_pte(paddr, flags);
	if ((flags & VM_MAP_FLAG_DEBUG) != 0) {
		printk("      L3_VIRT[L3_INDEX] = %llx\n", l3_virt[l3_idx]);
	}
	dsb_ishst(); // ensure visibility

	// TODO(bassosimone): as long as we are always creating new mappings
	// and context switching with a single process, we do not need to add
	// support for TLB invalidation to this code.
}

void __vm_switch(struct vm_root_pt root) {
	// 1. MAIR: idx0 Normal WBWA, idx1 Device-nGnRE
	uint64_t mair = (MAIR_ATTR_NORMAL_WBWA << 0) | (MAIR_ATTR_DEVICE_nGnRE << 8);
	printk("vm: msr_mair_el1 %llx\n", mair);
	msr_mair_el1(mair);
	isb();

	// 2. TCR: 39-bit VA for TTBR0/1, 4K granule, Inner WBWA, Inner-shareable
	const uint64_t T0SZ = 25, T1SZ = 25;
	const uint64_t IRGN_WBWA = 1, ORGN_WBWA = 1, SH_INNER = 3;
	const uint64_t TG0_4K = 0ULL << 14;
	const uint64_t TG1_4K = 2ULL << 30;
	const uint64_t IPS_40BIT = 2ull << 32;
	uint64_t tcr = 0 | (T0SZ) | (IRGN_WBWA << 8) | (ORGN_WBWA << 10) | (SH_INNER << 12) | TG0_4K | (T1SZ << 16) |
	               (IRGN_WBWA << 24) | (ORGN_WBWA << 26) | (SH_INNER << 28) | TG1_4K;
	tcr |= IPS_40BIT;
	printk("vm: msr_tcr_el1 %llx\n", tcr);
	msr_tcr_el1(tcr);
	isb();

	// 3. set TTBR0 to the kernel root
	printk("vm: msr_ttbr0_el1\n");
	msr_ttbr0_el1(root.table);
	isb();

	// 4. Enable MMU + caches
	uint64_t sctlr = mrs_sctlr_el1();
	sctlr |= (1ULL << 0) | (1ULL << 2) | (1ULL << 12); /* M: MMU enable; C: data cache; I: icache */
	printk("vm: msr_sctlr_el1: %llx\n", sctlr);
	msr_sctlr_el1(sctlr);
	isb();
}
