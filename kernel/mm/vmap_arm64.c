// File: kernel/mm/vmap_arm64.c
// Purpose: ARM64 bits of the vmap implementation.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/asm/arm64.h>	// for dsb_sy, etc.
#include <kernel/boot/boot.h>	// for __kernel_base
#include <kernel/core/assert.h> // for KERNEL_ASSERT
#include <kernel/core/printk.h> // for printk
#include <kernel/mm/mm.h>	// for mm_phys_page_alloc_many
#include <kernel/trap/trap.h>	// for trap_init_mm
#include <kernel/tty/uart.h>	// for uart_init_mm

// We're using identity mapping in this kernel
static inline void *__phys_to_virt(uint64_t paddr) {
	return (void *)paddr;
}

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

static uint64_t arm64_make_leaf_pte(mm_phys_addr_t paddr, mm_flags_t flags) {
	uint64_t pte = paddr & ARM64_PTE_ADDR_MASK;

	// Valid leaf
	pte |= ARM64_PTE_VALID | ARM64_PTE_TABLE | ARM64_PTE_AF;

	// Memory type + shareability
	if ((flags & MM_FLAG_DEVICE) != 0) {
		// Device-nGnRE is common for MMIO (AttrIdx=1), SH is ignored for Device but OUTER is fine
		pte |= ARM64_PTE_ATTRINDX(1) | ARM64_PTE_SH_OUTER;
		// Device memory MUST NOT be executable by kernel or user
		pte |= ARM64_PTE_UXN | ARM64_PTE_PXN;
	} else {
		// Normal WB WA cacheable (AttrIdx=0), Inner-shareable for coherency
		pte |= ARM64_PTE_ATTRINDX(0) | ARM64_PTE_SH_INNER;
	}

	const bool is_user = (flags & MM_FLAG_USER) != 0;
	const bool can_write = (flags & MM_FLAG_WRITE) != 0;
	const bool can_exec = (flags & MM_FLAG_EXEC) != 0;

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
		printk("mm: W|X is discouraged for %sspace\n", is_user ? "user" : "kernel");
	}

	// Mark user pages as non-global to protect
	// the memory assigned to each user
	if (is_user) {
		pte |= ARM64_PTE_nG;
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

	printk("      vaddr %llx => l1_idx %llx l2_idx %llx l3_idx %llx\n", vaddr, l1_idx, l2_idx, l3_idx);

	// TODO(bassosimone): refactor duplicated code into a single
	// function named ensure_table_entry, maybe.

	// Step 2: walk L1
	uint64_t *l1 = (uint64_t *)__phys_to_virt(table);
	if ((l1[l1_idx] & ARM64_PTE_VALID) == 0) {
		mm_phys_addr_t l2_phys = mm_phys_page_alloc_many(1);
		l1[l1_idx] = arm64_make_table_desc(l2_phys);
		dsb_ishst(); // ensure table write is visible
	}
	printk("      l1[l1_idx] = %llx\n", l1[l1_idx]);

	// Step 3: walk L2
	uint64_t *l2 = (uint64_t *)__phys_to_virt(l1[l1_idx] & ARM64_PTE_ADDR_MASK);
	if ((l2[l2_idx] & ARM64_PTE_VALID) == 0) {
		mm_phys_addr_t l3_phys = mm_phys_page_alloc_many(1);
		l2[l2_idx] = arm64_make_table_desc(l3_phys);
		dsb_ishst(); // ensure table write is visible
	}
	printk("      l2[l2_idx] = %llx\n", l2[l2_idx]);

	// Step 4: walk L3 (leaf)
	uint64_t *l3 = (uint64_t *)__phys_to_virt(l2[l2_idx] & ARM64_PTE_ADDR_MASK);
	uint64_t pte = arm64_make_leaf_pte(paddr, flags);
	l3[l3_idx] = pte;
	printk("      l3[l3_idx] = %llx\n", l3[l3_idx]);
	dsb_ishst(); // ensure page mapping is visible

	// TODO(bassosimone): as long as we are always creating new mappings
	// and context switching with a single process, we do not need to add
	// support for TLB invalidation in this code.
}

// The kernel root page table.
static uint64_t kernel_root_table;

void mm_init(void) {
	// 1) Root table for TTBR0 (kernel)
	kernel_root_table = mm_phys_page_alloc_many(1);
	printk("mm: kernel_root_table %llx\n", kernel_root_table);

	// 2) Map kernel sections (identity)
	printk("mm: mapping __kernel_base %llx -> __kernel_end %llx\n", __kernel_base, __kernel_end);
	mm_map_identity(kernel_root_table, (uint64_t)__kernel_base, (uint64_t)__kernel_end, MM_FLAG_EXEC);

	printk("mm: mapping __rodata_base %llx -> __rodata_end %llx\n", __rodata_base, __rodata_end);
	mm_map_identity(kernel_root_table, (uint64_t)__rodata_base, (uint64_t)__rodata_end, 0);

	printk("mm: mapping __data_base %llx -> __data_end %llx\n", __data_base, __data_end);
	mm_map_identity(kernel_root_table, (uint64_t)__data_base, (uint64_t)__data_end, MM_FLAG_WRITE);

	printk("mm: mapping __bss_base %llx -> __bss_end %llx\n", __bss_base, __bss_end);
	mm_map_identity(kernel_root_table, (uint64_t)__bss_base, (uint64_t)__bss_end, MM_FLAG_WRITE);

	// kernel stack: RW + NX
	printk("mm: mapping __stack_bottom %llx -> __stack_top %llx\n", __stack_bottom, __stack_top);
	mm_map_identity(kernel_root_table, (uint64_t)__stack_bottom, (uint64_t)__stack_top, MM_FLAG_WRITE);

	// 3) Map devices we actually use
	trap_init_mm();
	uart_init_mm();

	// 4) MAIR: idx0 Normal WBWA, idx1 Device-nGnRE
	uint64_t mair = (MAIR_ATTR_NORMAL_WBWA << 0) | (MAIR_ATTR_DEVICE_nGnRE << 8);
	printk("mm: msr_mair_el1 %llx\n", mair);
	msr_mair_el1(mair);
	isb();

	// 5) TCR: 39-bit VA for TTBR0/1, 4K granule, Inner WBWA, Inner-shareable
	const uint64_t T0SZ = 25, T1SZ = 25;
	const uint64_t IRGN_WBWA = 1, ORGN_WBWA = 1, SH_INNER = 3;
	const uint64_t TG0_4K = 0ULL << 14;
	const uint64_t TG1_4K = 2ULL << 30;
	const uint64_t IPS_40BIT = 2ull << 32;
	uint64_t tcr = 0 | (T0SZ) | (IRGN_WBWA << 8) | (ORGN_WBWA << 10) | (SH_INNER << 12) | TG0_4K |
		       (T1SZ << 16) | (IRGN_WBWA << 24) | (ORGN_WBWA << 26) | (SH_INNER << 28) | TG1_4K;
	tcr |= IPS_40BIT;
	printk("mm: msr_tcr_el1 %llx\n", tcr);
	msr_tcr_el1(tcr);
	isb();

	// 6) set TTBR0 to the kernel root
	printk("mm: msr_tbr0_el1\n");
	msr_ttbr0_el1(kernel_root_table);
	isb();

	// 7) Enable MMU + caches
	uint64_t sctlr = mrs_sctlr_el1();
	sctlr |= (1ULL << 0) | (1ULL << 2) | (1ULL << 12); /* M: MMU enable; C: data cache; I: icache */
	printk("mm: msr_sctlr_el1: %llx\n", sctlr);
	msr_sctlr_el1(sctlr);
	isb();
}

void mmap_identity(mm_phys_addr_t start, mm_phys_addr_t end, mm_flags_t flags) {
	mm_map_identity(kernel_root_table, start, end, flags);
}
