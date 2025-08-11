// File: kernel/trap/gicv2_arm64.c
// Purpose: GICv2 driver
// SPDX-License-Identifier: MIT

#include <kernel/asm/asm.h>		// for mmio_write_uint32
#include <kernel/core/assert.h>		// for KERNEL_ASSERT
#include <kernel/core/printk.h>		// for printk
#include <kernel/drivers/gicv2_arm64.h> // for struct gicv2_device
#include <kernel/mm/mm.h>		// for mmap_identity

#include <sys/types.h> // for uintptr_t

// Returns the limit of the GICC MMIO range given a specific base.
static inline uintptr_t gicc_memory_limit(uintptr_t base) {
	return base + 0x2000ULL;
}

// Returns the limit of the GICD MMIO range given a specific base.
static inline uintptr_t gicd_memory_limit(uintptr_t base) {
	return base + 0x10000ULL;
}

void gicv2_init_struct(struct gicv2_device *dev, uintptr_t gicc_base, uintptr_t gicd_base, const char *name) {
	dev->gicc_base = gicc_base;
	dev->gicd_base = gicd_base;
	dev->name = name;
}

void gicv2_init_mm(struct gicv2_device *dev) {
	uintptr_t gicc_limit = gicc_memory_limit(dev->gicc_base);
	printk("%s: gicv2: mmap_identity GICC_BASE %llx - %llx\n", dev->name, dev->gicc_base, gicc_limit);
	mmap_identity(dev->gicc_base, gicc_limit, MM_FLAG_DEVICE | MM_FLAG_WRITE);

	uintptr_t gicd_limit = gicd_memory_limit(dev->gicd_base);
	printk("%s: gicv2: mmap_identity GICD_BASE %llx - %llx\n", dev->name, dev->gicd_base, gicd_limit);
	mmap_identity(dev->gicd_base, gicd_limit, MM_FLAG_DEVICE | MM_FLAG_WRITE);
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

void gicv2_enable_ppi(struct gicv2_device *dev, uint32_t id, uint8_t prio) {
	// Make sure we're not going beyond the expected memory region.
	KERNEL_ASSERT(id >= 16 && id <= 31);

	printk("%s: gicv2: disabling %u\n", dev->name, id);
	mmio_write_uint32(gicd_icenabler_addr(dev->gicd_base, 0), (1u << id));

	printk("%s: gicv2: clear pending IRQs for %u\n", dev->name, id);
	mmio_write_uint32(gicd_icpendr_addr(dev->gicd_base, 0), (1u << id));

	printk("%s: gicv2: setting priority of %u to %u\n", dev->name, id, (unsigned)prio);
	mmio_write_uint8(gicd_ipriorityr_byte_addr(dev->gicd_base, id), prio);

	printk("%s: gicv2: enabling %u\n", dev->name, id);
	mmio_write_uint32(gicd_isenabler_addr(dev->gicd_base, 0), (1u << id));
}

void gicv2_enable_spi_level_cpu0(struct gicv2_device *dev, uint32_t id, uint8_t prio) {
	// Added a random not-so-large value here
	KERNEL_ASSERT(id >= 32 && id <= 256);

	// Compute the n and the bit from the IRQ id
	const uint32_t n = id / 32;   // 1
	const uint32_t bit = id % 32; // 1

	printk("%s: gicv2: disabling %u\n", dev->name, id);
	mmio_write_uint32(gicd_icenabler_addr(dev->gicd_base, n), (1u << bit));

	printk("%s: gicv2: setting priority of %u to %u\n", dev->name, id, (unsigned)prio);
	mmio_write_uint8(gicd_ipriorityr_byte_addr(dev->gicd_base, id), prio);

	printk("%s: gicv2: routing %u to CPU0\n", dev->name, id);
	mmio_write_uint8(gicd_itargetsr_byte_addr(dev->gicd_base, id), 0x01);

	printk("%s: gicv2: setting level-triggered IRQs for %u\n", dev->name, id);
	const uint32_t idx = id / 16;
	const uint32_t shift = (id % 16) * 2;
	uint32_t cfgr = mmio_read_uint32(gicd_icfgr_addr(dev->gicd_base, idx));
	cfgr &= ~(3u << shift);
	mmio_write_uint32(gicd_icfgr_addr(dev->gicd_base, idx), cfgr);

	printk("%s: gicv2: clear pending IRQs for %u\n", dev->name, id);
	mmio_write_uint32(gicd_icpendr_addr(dev->gicd_base, n), (1u << bit));

	printk("%s: gicv2: enabling %u\n", dev->name, id);
	mmio_write_uint32(gicd_isenabler_addr(dev->gicd_base, n), (1u << bit));
}

void gicv2_reset(struct gicv2_device *dev) {
	printk("%s: gicv2: disabling CPU interface\n", dev->name);
	mmio_write_uint32(gicc_ctrl_addr(dev->gicc_base), 0);

	printk("%s: gicv2: disabling distributor\n", dev->name);
	mmio_write_uint32(gicd_ctrl_addr(dev->gicd_base), 0);

	printk("%s: gicv2: setting priority mask to 0xFF\n", dev->name);
	mmio_write_uint32(gicc_pmr_addr(dev->gicc_base), 0xFF);

	printk("%s: gicv2: disabling binary point split\n", dev->name);
	mmio_write_uint32(gicc_bpr_addr(dev->gicc_base), 0);

	// TODO(bassosimone): clear and disable shared peripherals (SPIs)
}

void gicv2_enable(struct gicv2_device *dev) {
	printk("%s: gicv2: enabling the distributor\n", dev->name);
	mmio_write_uint32(gicd_ctrl_addr(dev->gicd_base), 1);

	printk("%s: gicv2: enabling the CPU interface\n", dev->name);
	mmio_write_uint32(gicc_ctrl_addr(dev->gicc_base), 1);
}

bool gicv2_acknowledge_irq(struct gicv2_device *dev, uint32_t *iar, uint32_t *id) {
	// Acknowledge the IRQ and get the context
	*iar = mmio_read_uint32(gicc_iar_addr(dev->gicc_base));

	// Extract the interrupt ID from the context
	*id = *iar & 0x3FFu;

	// Handle spurious IRQ
	return *id < 1020;
}

void gicv2_end_of_interrupt(struct gicv2_device *dev, uint32_t iar) {
	mmio_write_uint32(gicc_eoir_addr(dev->gicc_base), iar);
}
