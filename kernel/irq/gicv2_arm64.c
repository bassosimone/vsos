// File: kernel/boot/irq_arm64.c
// Purpose: GICv2 driver
// SPDX-License-Identifier: MIT

#include <kernel/asm/arm64.h>	// for dsb_sy, etc.
#include <kernel/boot/boot.h>	// for __vectors_el1
#include <kernel/core/printk.h> // for printk.
#include <kernel/irq/irq.h>	// for irq_init
#include <kernel/mm/vmap.h>	// for mmap_identity
#include <kernel/sched/sched.h> // for sched_clock_irq

// Base and limit memory addresses for the GICC.
#define GICC_BASE 0x08010000UL
#define GICC_LIMIT GICC_BASE + 0x2000ULL

// Base and limit memory addresses for the GICD.
#define GICD_BASE 0x08000000UL
#define GICD_LIMIT GICD_BASE + 0x10000ULL

// The ARM Generic Timer (physical EL1) PPI is INTID 30 on GIC (per-cpu)
#define IRQ_PPI_CNTP 30u

void irq_init_mm(void) {
	printk("irq0: mmap_identity GICD_BASE %llx - %llx\n", GICD_BASE, GICD_LIMIT);
	mmap_identity(GICD_BASE, GICD_LIMIT, MM_FLAG_DEVICE | MM_FLAG_WRITE);

	printk("irq0: mmap_identity GICC_BASE %llx - %llx\n", GICC_BASE, GICC_LIMIT);
	mmap_identity(GICC_BASE, GICC_LIMIT, MM_FLAG_DEVICE | MM_FLAG_WRITE);
}

// GICC_CTRL: CPU interface control register.
static inline volatile uint32_t *gicc_ctrl_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x000);
}

// GICD_CTRL: distributor control register.
static inline volatile uint32_t *gicd_ctrl_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x000);
}

// GICC_PMR: CPU interface interrupt priority mask register.
static inline volatile uint32_t *gicc_pmr_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x004);
}

// GICC_BPR: CPU interface binary point register.
static inline volatile uint32_t *gicc_bpr_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x008);
}

// GICD_ICENABLER: distributor interrupt clear-enable register.
static inline volatile uint32_t *gicd_icenabler_addr(uintptr_t base, size_t n) {
	return (volatile uint32_t *)(base + 0x180 + 4 * (n));
}

// GICD_ICPENDR: distributor interrupt clear-pending register.
static inline volatile uint32_t *gicd_icpendr_addr(uintptr_t base, size_t n) {
	return (volatile uint32_t *)(base + 0x280 + 4 * (n));
}

// GICD_ISENABLER: distributor interrupt set-enable register.
static inline volatile uint32_t *gicd_isenabler_addr(uintptr_t base, size_t n) {
	return (volatile uint32_t *)(base + 0x100 + 4 * (n));
}

// GICC_EOIR: CPU interface end of interrupt register.
static inline volatile uint32_t *gicc_eoir_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x010);
}

// GICC_IAR: CPU interface interrupt acknowledgment register.
static inline volatile uint32_t *gicc_iar(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x00C);
}

static inline void __enable_timer_ppi(void) {
	printk("irq0: enabling interrupt PPI %u (timer)\n", IRQ_PPI_CNTP);
	*gicd_icenabler_addr(GICD_BASE, 0) = (1u << IRQ_PPI_CNTP);
	*gicd_icpendr_addr(GICD_BASE, 0) = (1u << IRQ_PPI_CNTP);
	*gicd_isenabler_addr(GICD_BASE, 0) = (1u << IRQ_PPI_CNTP);
}

void irq_init(void) {
	// Set the vector interrupt table
	msr_vbar_el1((uint64_t)__vectors_el1);
	isb();

	// Disable the CPU interface and the distributor.
	printk("irq0: disabling CPU interface and distributor\n");
	*gicc_ctrl_addr(GICC_BASE) = 0;
	*gicd_ctrl_addr(GICD_BASE) = 0;
	dsb_sy();

	// Set priority mask to allow all interrupts and disable binary point split.
	printk("irq0: setting priority mask to allow all interrupts\n");
	*gicc_pmr_addr(GICC_BASE) = 0xFF;
	*gicc_bpr_addr(GICC_BASE) = 0;

	// Enable devices
	__enable_timer_ppi();

	// Enable distributor and CPU interface again.
	printk("irq0: enabling CPU interface and distributor\n");
	*gicd_ctrl_addr(GICD_BASE) = 1;
	*gicc_ctrl_addr(GICC_BASE) = 1;
	dsb_sy();
	isb();

	// Start IRQ for other subsystems
	sched_clock_init_irq();
}

void irq_handle(uintptr_t frame) {
	(void)frame;

	// Acknowledge the IRQ and get the context
	uint32_t iar = *gicc_iar(GICC_BASE);
	dsb_sy();

	// Extract the interrupt ID from the context
	uint32_t irqid = iar & 0x3FFu;

	// Handle spurious IRQ
	if (irqid >= 1020) {
		return;
	}

	// Handle each IRQ type
	switch (irqid) {
	case IRQ_PPI_CNTP:
		sched_clock_irq();
		break;

	default:
		printk("unhandled IRQ %u\n", irqid);
	}

	// We're done handling this interrupt
	*gicc_eoir_addr(GICC_BASE) = iar;
	dsb_sy();
	isb();
}
