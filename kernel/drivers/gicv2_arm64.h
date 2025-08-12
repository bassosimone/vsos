// File: kernel/trap/gicv2_arm64.c
// Purpose: GICv2 driver
// SPDX-License-Identifier: MIT
#ifndef KERNEL_DRIVERS_GICV2_H
#define KERNEL_DRIVERS_GICV2_H

#include <kernel/mm/vm.h> // for struct vm_root_pt

#include <sys/types.h> // for uintptr_t

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
void gicv2_init_struct(struct gicv2_device *dev, uintptr_t gicc_base, uintptr_t gicd_base, const char *name);

// Initialize memory mapping for the GICv2 driver.
//
// Called by the trap subsystem.
//
// Requires gicv2_init_struct first.
void gicv2_init_mm(struct gicv2_device *dev, struct vm_root_pt root);

// Enables the given private-peripheral interrupt (i.e., per-CPU interface).
//
// The clock, for example, belongs to this class.
//
// Requires gicv2_reset first and should happen before gicv2_enable.
void gicv2_enable_ppi(struct gicv2_device *dev, uint32_t id, uint8_t prio);

// Enables the given shared-peripheral interrupt (i.e., shared accross the CPUs).
//
// The PL011, for example, belongs to this class.
//
// Requires gicv2_reset first and should happen before gicv2_enable.
void gicv2_enable_spi_level_cpu0(struct gicv2_device *dev, uint32_t id, uint8_t prio);

// Returns the GICv2 into a know state with all interrupts disabled.
//
// Should be invoked first, before starting to program the device.
void gicv2_reset(struct gicv2_device *dev);

// Enables both the CPU interface and the distributor.
//
// Requires gicv2_reset first. Should happen after enabling the various IRQs.
void gicv2_enable(struct gicv2_device *dev);

// Returns true and a valid `iar` or false if the IRQ is a spurious one.
//
// Both `iar` and `id` will always be initialized to some value.
bool gicv2_acknowledge_irq(struct gicv2_device *dev, uint32_t *iar, uint32_t *id);

// Notify that we are done handling this interrupt.
void gicv2_end_of_interrupt(struct gicv2_device *dev, uint32_t iar);

#endif // KERNEL_DRIVERS_GICV2_H
