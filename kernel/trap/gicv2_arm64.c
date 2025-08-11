// File: kernel/trap/gicv2_arm64.c
// Purpose: GICv2 driver
// SPDX-License-Identifier: MIT

#include <kernel/asm/arm64.h>	    // for dsb_sy, etc.
#include <kernel/boot/boot.h>	    // for __vectors_el1
#include <kernel/core/printk.h>	    // for printk.
#include <kernel/mm/mm.h>	    // for mmap_identity
#include <kernel/sched/sched.h>	    // for sched_clock_irq
#include <kernel/trap/trap.h>	    // for trap_init_mm
#include <kernel/trap/trap_arm64.h> // for __trap_isr
#include <kernel/tty/uart.h>	    // for uart_init_irq

// Returns the limit of the GICC MMIO range given a specific base.
static inline uintptr_t gicc_memory_limit(uintptr_t base) {
	return base + 0x2000ULL;
}

// Returns the limit of the GICD MMIO range given a specific base.
static inline uintptr_t gicd_memory_limit(uintptr_t base) {
	return base + 0x10000ULL;
}

// GICv2 device.
//
// Initialize using gicv2_init_struct.
struct gicv2_device {
	// GICC base MMIO address.
	uintptr_t gicc_base;

	// GICD base MMIO address.
	uintptr_t gicd_base;

	// name is the corresponding device name.
	const char *name;
};

// Initialize the gicv2_device structure.
//
// You retain ownership of the device structure and of the name.
static inline void
gicv2_init_struct(struct gicv2_device *dev, uintptr_t gicc_base, uintptr_t gicd_base, const char *name) {
	dev->gicc_base = gicc_base;
	dev->gicd_base = gicd_base;
	dev->name = name;
}

// Initialize memory mapping for the GICv2 driver.
//
// Called by the trap subsystem.
//
// Requires gicv2_init_struct first.
static inline void gicv2_init_mm(struct gicv2_device *dev) {
	uintptr_t gicc_limit = gicc_memory_limit(dev->gicc_base);
	printk("%s: mmap_identity GICC_BASE %llx - %llx\n", dev->name, dev->gicc_base, gicc_limit);
	mmap_identity(dev->gicc_base, gicc_limit, MM_FLAG_DEVICE | MM_FLAG_WRITE);

	uintptr_t gicd_limit = gicd_memory_limit(dev->gicd_base);
	printk("%s: mmap_identity GICD_BASE %llx - %llx\n", dev->name, dev->gicd_base, gicd_limit);
	mmap_identity(dev->gicd_base, gicd_limit, MM_FLAG_DEVICE | MM_FLAG_WRITE);
}

// Base and limit memory addresses for the GICC.
#define GICC_BASE 0x08010000UL

// Base and limit memory addresses for the GICD.
#define GICD_BASE 0x08000000UL

// The ARM Generic Timer (physical EL1) PPI is INTID 30 on GIC (per-cpu)
#define IRQ_PPI_CNTP 30u

// The UART0 (PL011) on QEMU virt
#define UART0_INTID 33u

// The global irq0 device driver attached to the GICCv2.
struct gicv2_device irq0;

void trap_init_mm(void) {
	gicv2_init_struct(&irq0, GICC_BASE, GICD_BASE, "irq0");
	gicv2_init_mm(&irq0);
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
static inline volatile uint32_t *gicc_iar_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x00C);
}

// GICD_IPRIORITYR: byte-addressed priority registers (one byte per INTID)
static inline volatile uint8_t *gicd_ipriorityr_byte_addr(uintptr_t base, size_t i) {
	return (volatile uint8_t *)(base + 0x400 + i);
}

// GICD_ITARGETSR: byte-addressed target CPU registers (one byte per INTID; SPIs only)
static inline volatile uint8_t *gicd_itargetsr_byte_addr(uintptr_t base, size_t i) {
	return (volatile uint8_t *)(base + 0x800 + i);
}

// GICD_ICFGR: config (edge/level) registers (2 bits per INTID)
static inline volatile uint32_t *gicd_icfgr_addr(uintptr_t base, size_t n) {
	return (volatile uint32_t *)(base + 0xC00 + 4 * n);
}

static inline void __enable_timer_irq(void) {
	printk("irq0: enabling interrupt PPI %u (timer)\n", IRQ_PPI_CNTP);
	*gicd_icenabler_addr(GICD_BASE, 0) = (1u << IRQ_PPI_CNTP);
	*gicd_icpendr_addr(GICD_BASE, 0) = (1u << IRQ_PPI_CNTP);
	*gicd_isenabler_addr(GICD_BASE, 0) = (1u << IRQ_PPI_CNTP);
}

static inline void __enable_uart_irq(void) {
	const uint32_t id = UART0_INTID; // 33
	const uint32_t n = id / 32;	 // 1
	const uint32_t bit = id % 32;	 // 1

	// Priority (lower is higher priority)
	*gicd_ipriorityr_byte_addr(GICD_BASE, id) = 0x80;

	// Route to CPU0 (bit0 = CPU0)
	*gicd_itargetsr_byte_addr(GICD_BASE, id) = 0x01;

	// Level-sensitive (00b)
	const uint32_t idx = id / 16;	      // 2
	const uint32_t shift = (id % 16) * 2; // 2
	uint32_t cfgr = *gicd_icfgr_addr(GICD_BASE, idx);
	cfgr &= ~(3u << shift); // clear to 00b (level)
	*gicd_icfgr_addr(GICD_BASE, idx) = cfgr;

	// Clean pending, (re)disable, then enable
	*gicd_icenabler_addr(GICD_BASE, n) = (1u << bit);
	*gicd_icpendr_addr(GICD_BASE, n) = (1u << bit);
	*gicd_isenabler_addr(GICD_BASE, n) = (1u << bit);
}

void trap_init_irqs(void) {
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

	// Program devices (group/prio/route/trigger and set-enable)
	__enable_timer_irq();
	__enable_uart_irq();

	// Enable distributor and CPU interface (single-group NS-only flow)
	printk("irq0: enabling CPU interface and distributor\n");
	*gicd_ctrl_addr(GICD_BASE) = 1;
	*gicc_ctrl_addr(GICC_BASE) = 1;
	dsb_sy();
	isb();

	// Start IRQ for other subsystems
	sched_clock_init_irqs();
	uart_init_irqs();
}

void __trap_isr(struct trap_frame *frame) {
	(void)frame;

	// Acknowledge the IRQ and get the context
	uint32_t iar = *gicc_iar_addr(GICC_BASE);
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
		sched_clock_isr();
		break;

	case UART0_INTID:
		uart_isr();
		break;

	default:
		// Mask unexpected lines so they can't storm while debugging.
		*gicd_icenabler_addr(GICD_BASE, (irqid / 32)) = (1u << (irqid % 32));
		break;
	}

	// We're done handling this interrupt
	*gicc_eoir_addr(GICC_BASE) = iar;
	dsb_sy();
	isb();
}
