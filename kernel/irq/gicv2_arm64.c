// File: kernel/boot/irq_arm64.c
// Purpose: GICv2 driver
// SPDX-License-Identifier: MIT

#include <kernel/asm/arm64.h>	// for dsb_sy, etc.
#include <kernel/boot/boot.h>	// for __vectors_el1
#include <kernel/core/printk.h> // for printk.
#include <kernel/irq/irq.h>	// for irq_init
#include <kernel/mm/vmap.h>	// for mmap_identity
#include <kernel/sched/sched.h> // for sched_clock_irq

// We're using GICv2, which should be available in `qemu-system-aarch64 -M virt`.
#define GICD_BASE 0x08000000UL
#define GICD_LIMIT GICD_BASE + 0x10000ULL

#define GICC_BASE 0x08010000UL
#define GICC_LIMIT GICC_BASE + 0x2000ULL

#define GICR_BASE 0x080A0000ULL
#define GICR_LIMIT GICR_BASE + 0x20000ULL

// Distributor
#define GICD_CTLR (*(volatile uint32_t *)(GICD_BASE + 0x000))
#define GICD_ISENABLER(n) (*(volatile uint32_t *)(GICD_BASE + 0x100 + 4 * (n)))
#define GICD_ICENABLER(n) (*(volatile uint32_t *)(GICD_BASE + 0x180 + 4 * (n)))
#define GICD_IPRIORITYR(n) (*(volatile uint32_t *)(GICD_BASE + 0x400 + 4 * (n)))
#define GICD_ISPENDR(n) (*(volatile uint32_t *)(GICD_BASE + 0x200 + 4 * (n)))
#define GICD_ICPENDR(n) (*(volatile uint32_t *)(GICD_BASE + 0x280 + 4 * (n)))

// CPU interface
#define GICC_CTLR (*(volatile uint32_t *)(GICC_BASE + 0x000))
#define GICC_PMR (*(volatile uint32_t *)(GICC_BASE + 0x004))
#define GICC_BPR (*(volatile uint32_t *)(GICC_BASE + 0x008))
#define GICC_IAR (*(volatile uint32_t *)(GICC_BASE + 0x00C))
#define GICC_EOIR (*(volatile uint32_t *)(GICC_BASE + 0x010))

// The ARM Generic Timer (physical EL1) PPI is INTID 30 on GIC (per-cpu)
#define IRQ_PPI_CNTP 30u

void irq_init_mm(void) {
	printk("irq0: mmap_identity GICD_BASE %llx - %llx\n", GICD_BASE, GICD_LIMIT);
	mmap_identity(GICD_BASE, GICD_LIMIT, MM_FLAG_DEVICE | MM_FLAG_WRITE);

	printk("irq0: mmap_identity GICC_BASE %llx - %llx\n", GICC_BASE, GICC_LIMIT);
	mmap_identity(GICC_BASE, GICC_LIMIT, MM_FLAG_DEVICE | MM_FLAG_WRITE);

	printk("irq0: mmap_identity GICR_BASE %llx - %llx\n", GICR_BASE, GICR_LIMIT);
	mmap_identity(GICR_BASE, GICR_LIMIT, MM_FLAG_DEVICE | MM_FLAG_WRITE);
}

void irq_init(void) {
	// Set the vector interrupt table
	msr_vbar_el1((uint64_t)__vectors_el1);

	// Mask CPU interface, then enable distributor, then CPU interface
	GICC_CTLR = 0; // disable CPU IF
	GICD_CTLR = 0; // disable dist
	dsb_sy();

	// Set priority mask to allow all (0xFF = lowest priority accepted)
	GICC_PMR = 0xFF;
	GICC_BPR = 0; // no binary point split

	// Enable timer PPI (banked per CPU): INTID 30 lives in ISENABLER0 (0..31)
	GICD_ICENABLER(0) = (1u << IRQ_PPI_CNTP);
	GICD_ICPENDR(0) = (1u << IRQ_PPI_CNTP); // clear any pending
	GICD_ISENABLER(0) = (1u << IRQ_PPI_CNTP);

	// Enable distributor and CPU interface
	GICD_CTLR = 1; // enable dist
	GICC_CTLR = 1; // enable CPU IF
	dsb_sy();

	// Let the user know what we have done
	printk("irq0: GICv2 enabled, PPI %u unmasked\n", IRQ_PPI_CNTP);
}

static inline uint32_t gicv2_ack(void) {
	return GICC_IAR;
}

static inline void gicv2_eoi(unsigned iar) {
	GICC_EOIR = iar;
}

void irq_handle(uintptr_t frame) {
	// We will use the frame in the future to context switch
	(void)frame;

	// Acknowledge the IRQ and get the context
	unsigned iar = gicv2_ack();
	unsigned irqid = iar & 0x3FFu;

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
	gicv2_eoi(iar);
}
