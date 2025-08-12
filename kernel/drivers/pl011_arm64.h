// File: kernel/drivers/pl011_arm64.h
// Purpose: AMBA PL011 UART device driver.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_DRIVERS_PL011_ARM64_H
#define KERNEL_DRIVERS_PL011_ARM64_H

#include <kernel/core/ringbuf.h>  // for struct ringbuf
#include <kernel/core/spinlock.h> // for struct spinlock

#include <sys/types.h> // for uintptr_t

// AMBA PL011 UART device.
//
// Initialize using pl011_init_struct.
struct pl011_device {
	// The base MMIO pointer.
	uintptr_t base;

	// The associated device name.
	const char *name;

	// Internal fields.
	volatile uint64_t __has_interrupts;
	struct ringbuf __rxbuf;
	struct spinlock __rxlock;
	struct spinlock __txlock;
};

// Initialize the pl011_device struct.
//
// You retain ownership of the device struct and of the name.
void pl011_init_struct(struct pl011_device *dev, uintptr_t mmio_base, const char *name);

// Early initialization for the PL011 driver.
//
// Called during early boot typically by the tty driver.
//
// Configures the device to operate w/o MMIO and interrupts.
//
// Requires pl011_init_struct.
void pl011_init_early(struct pl011_device *dev);

// Initialize memory mapping for the PL011 driver.
//
// Called during early boot typically by the tty driver.
//
// Uses the mm subsystem to setup a memory map.
//
// Requires pl011_init_early.
void pl011_init_mm(struct pl011_device *dev, uintptr_t root_table);

// Initialize IRQs for the PL011 driver.
//
// Called during early boot typically by the tty driver.
//
// Uses the irq subsystem to setup the IRQs.
//
// Requires pl011_init_mm.
void pl011_init_irqs(struct pl011_device *dev);

// PL011 interrupt service request.
//
// Called by the irq subsystem via the tty driver.
void pl011_isr(struct pl011_device *dev);

// Allows reading bytes from the PL011.
//
// The count is silently truncated to SSIZE_MAX if bigger.
//
// The O_NONBLOCK flag allows for nonblocking ops.
//
// Returns the number of bytes read or a negative errno value.
//
// This function is safe to be called by multiple threads concurrently
// however threads running at IRQ level MUST use O_NONBLOCK.
//
// W/o O_NONBLOCK this function blocks until interrupts are enabled.
ssize_t pl011_recv(struct pl011_device *dev, char *buf, size_t count, uint32_t flags);

// Allows writing bytes to the PL011.
//
// The count is silently truncated to SSIZE_MAX if bigger.
//
// The O_NONBLOCK flag allows for nonblocking ops.
//
// Returns the number of bytes written or a negative errno value.
//
// This function is safe to be called by multiple threads concurrently
// however threads running at IRQ level MUST use O_NONBLOCK.
//
// This function falls back to cooperative multitasking if
// we have not enabled interrupts for the PL011 yet.
ssize_t pl011_send(struct pl011_device *dev, const char *buf, size_t count, uint32_t flags);

#endif // KERNEL_DRIVERS_PL011_ARM64_H
