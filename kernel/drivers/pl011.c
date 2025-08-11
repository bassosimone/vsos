// File: kernel/drivers/pl011.c
// Purpose: AMBA PL011 UART device driver.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/asm/asm.h>	  // for mmio_write_uint32
#include <kernel/core/printk.h>	  // for printk
#include <kernel/core/ringbuf.h>  // for struct ringbuf
#include <kernel/core/spinlock.h> // for struct spinlock
#include <kernel/drivers/pl011.h> // for struct pl011_device
#include <kernel/mm/mm.h>	  // for mmap_identity
#include <kernel/sched/sched.h>	  // for sched_thread_suspend

#include <sys/errno.h> // for EAGAIN
#include <sys/fcntl.h> // for O_NONBLOCK

#include <string.h> // for __bzero_unaligned

// Returns the limit of the MMIO range given a specific base.
static inline uintptr_t memory_limit(uintptr_t base) {
	return base + 0x1000ULL;
}

// DR: data register.
static inline volatile uint32_t *dr_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x00);
}

// CR: enable UART, TX and RX.
static inline volatile uint32_t *cr_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x30);
}

// IMSC: interrupt mask set/clear register.
static inline volatile uint32_t *imsc_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x38);
}

// ICR: interrupt clear (i.e., acknowledge) register.
static inline volatile uint32_t *icr_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x44);
}

// UARTCR bit to enable the device.
#define UARTCR_UARTEN (1 << 0)

// UARTCR bit to enable receiving.
#define UARTCR_RXE (1 << 8)

// UARTCR bit to enable transmitting.
#define UARTCR_TXE (1 << 9)

// Bitmask used to clear all possible interrupt sources.
#define UARTICR_CLR_ALL 0x7FF

void pl011_init_struct(struct pl011_device *dev, uintptr_t base, const char *device_name) {
	__bzero_unaligned(dev, sizeof(*dev));
	dev->base = base;
	dev->name = device_name;
}

void pl011_init_early(struct pl011_device *dev) {
	// Disable UART and "push" changes
	mmio_write_uint32(cr_addr(dev->base), 0);

	// Mask all IRQs
	mmio_write_uint32(imsc_addr(dev->base), 0x0);

	// Clear any pending IRQs
	mmio_write_uint32(icr_addr(dev->base), UARTICR_CLR_ALL);

	// Enable the device, receiving, and sending.
	mmio_write_uint32(cr_addr(dev->base), UARTCR_UARTEN | UARTCR_RXE | UARTCR_TXE);

	// Let the user know what we did.
	printk("%s: UARTCR |= UARTEN | RXE | TXE\n", dev->name);
}

void pl011_init_mm(struct pl011_device *dev) {
	uintptr_t limit = memory_limit(dev->base);
	printk("%s: mmap_identity %llx - %llx\n", dev->name, dev->base, limit);
	mmap_identity(dev->base, limit, MM_FLAG_DEVICE | MM_FLAG_WRITE);
}

// UARTCLR_H bit to enable the FIFO.
#define UARTLCR_H_FEN (1u << 4)

// Unified interrupt bit positions (shared by RIS/MIS/IMSC/ICR)
#define UARTINT_RX (1u << 4)  /* Receive */
#define UARTINT_TX (1u << 5)  /* Transmit */
#define UARTINT_RT (1u << 6)  /* RX timeout */
#define UARTINT_FE (1u << 7)  /* Framing error */
#define UARTINT_PE (1u << 8)  /* Parity error */
#define UARTINT_BE (1u << 9)  /* Break error */
#define UARTINT_OE (1u << 10) /* Overrun */

// UARTCLR_H: line control register high part.
static inline volatile uint32_t *clr_h_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x2C);
}

// UARTIFLS: interrupt FIFO level select register.
static inline volatile uint32_t *ifls_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x34);
}

static inline void set_has_interrupts(struct pl011_device *dev) {
	__atomic_store_n(&dev->__has_interrupts, 1, __ATOMIC_RELEASE);
}

static inline bool has_enabled_interrupts(struct pl011_device *dev) {
	return __atomic_load_n(&dev->__has_interrupts, __ATOMIC_ACQUIRE) != 0;
}

void pl011_init_irqs(struct pl011_device *dev) {
	// Enable the FIFO behavior
	mmio_write_uint32(clr_h_addr(dev->base), (mmio_read_uint32(clr_h_addr(dev->base)) | UARTLCR_H_FEN));

	// Trigger interrupts when the RX level is 1/8 and the TX level is 1/8
	mmio_write_uint32(ifls_addr(dev->base), 0);

	// Defensively clear all potentially pending interrupts.
	mmio_write_uint32(icr_addr(dev->base), UARTICR_CLR_ALL);

	// Select the events to receive notifications about.
	mmio_write_uint32(imsc_addr(dev->base), UARTINT_RX | UARTINT_RT | UARTINT_OE);

	// Let the user know what we did *before* turning interrupts on.
	printk("%s: UARTIMSC |= RX | RT | OE\n", dev->name);

	// Ensure we all know we have interrupts.
	set_has_interrupts(dev);
}

// UARTFR: flags register.
static inline volatile uint32_t *fr_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x18);
}

// Transmit FIFO full
#define UARTFR_TXFF (1 << 5)

// Receive FIFO empty
#define UARTFR_RXFE (1 << 4)

// Return whether the UART is readable.
static inline bool is_readable(uintptr_t base) {
	return (mmio_read_uint32(fr_addr(base)) & UARTFR_RXFE) == 0;
}

// Return whether the UART is writable.
static inline bool is_writable(uintptr_t base) {
	return (mmio_read_uint32(fr_addr(base)) & UARTFR_TXFF) == 0;
}

// UARTMIS: masked interrupt status register.
static inline volatile uint32_t *mis_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x40);
}

void pl011_isr(struct pl011_device *dev) {
	uint32_t mis = mmio_read_uint32(mis_addr(dev->base));

	// Handle the case of the UART being readable
	if ((mis & (UARTINT_RX | UARTINT_RT | UARTINT_OE)) != 0) {
		// Drain the RX FIFO first including per-byte flags
		while (is_readable(dev->base)) {
			uint16_t value = (uint16_t)(mmio_read_uint32(dr_addr(dev->base)) & 0x0FFF);
			(void)ringbuf_push(&dev->__rxbuf, value);
		}

		// Clear RX-related causes and stop the interrupt from firing
		uint32_t mask = UARTINT_RX | UARTINT_RT | UARTINT_FE | UARTINT_PE | UARTINT_BE | UARTINT_OE;
		mmio_write_uint32(icr_addr(dev->base), mask);

		// Wake up any sleeping thread
		sched_thread_resume_all(SCHED_THREAD_WAIT_UART_READABLE);
	}

	// Handle the case of the UART being writable.
	if ((mis & UARTINT_TX) != 0) {
		// Acknowledge the TX interrupt
		mmio_write_uint32(icr_addr(dev->base), UARTINT_TX);

		// Mask the interrupt to avoid level-triggered interrupt storms.
		mmio_write_uint32(imsc_addr(dev->base),
				  (mmio_read_uint32(imsc_addr(dev->base)) & ~UARTINT_TX));

		// Let user space know it can send more
		sched_thread_resume_all(SCHED_THREAD_WAIT_UART_WRITABLE);
	}
}

ssize_t pl011_recv(struct pl011_device *dev, char *buf, size_t count, uint32_t flags) {
	// Defend against return value overflow
	count = (count <= SSIZE_MAX) ? count : SSIZE_MAX;

	// Attempt to read all the requested chars
	size_t off = 0;
	for (;;) {
		// Eventually stop reading
		if (off >= count) {
			return (ssize_t)off;
		}

		// Grab the spinlock to protect against multiple readers
		// and yield here awaiting for it to become available
		while (spinlock_try_acquire(&dev->__rxlock) != 0) {
			if ((flags & O_NONBLOCK) != 0) {
				return (off <= 0) ? -EAGAIN : (ssize_t)off;
			}
			sched_thread_yield();
		}

		// Attempt to read from the ring buffer
		uint16_t data = 0;
		bool success = ringbuf_pop(&dev->__rxbuf, &data);

		// Drop the spinlock now that we've accessed the buffer
		spinlock_release(&dev->__rxlock);

		// Check whether we succeeded
		if (success) {
			// Separate actual data from error flags.
			uint8_t err = (data & 0x0F00) >> 8;
			if (err != 0) {
				return (off <= 0) ? -EIO : (ssize_t)off;
			}
			buf[off++] = (char)(data & 0xFF);
			continue;
		}

		// Handle the nonblocking read case
		if ((flags & O_NONBLOCK) != 0) {
			return (off <= 0) ? -EAGAIN : (ssize_t)off;
		}

		// Suspend until we have data to read
		sched_thread_suspend(SCHED_THREAD_WAIT_UART_READABLE);
	}
}

ssize_t pl011_send(struct pl011_device *dev, const char *buf, size_t count, uint32_t flags) {
	// Ensure we're not going to overflow the return value
	count = (count <= SSIZE_MAX) ? count : SSIZE_MAX;

	// Count the number of characters sent
	size_t tot = 0;

	for (;;) {
		// Grab the lock granting us the permission to transmit,
		// however, be careful with O_NONBLOCK users.
		while (spinlock_try_acquire(&dev->__txlock) != 0) {
			if ((flags & O_NONBLOCK) != 0) {
				return (tot <= 0) ? -EAGAIN : (ssize_t)tot;
			}
			sched_thread_yield();
		}

		// Awesome, now send as much as possible until the
		// device tells us that it has enough data
		while (is_writable(dev->base) && tot < count) {
			mmio_write_uint32(dr_addr(dev->base), (uint32_t)((uint8_t)buf[tot++]));
		}

		// If everything has been sent, our job is done
		if (tot >= count) {
			spinlock_release(&dev->__txlock);
			return (ssize_t)tot;
		}

		// If we are nonblocking, this is the time where we'd block
		if ((flags & O_NONBLOCK) != 0) {
			spinlock_release(&dev->__txlock);
			return (tot <= 0) ? -EAGAIN : (ssize_t)tot;
		}

		// If we are without interrupts and we are allowed to
		// block, the best we can do is yield the CPU.
		if (!has_enabled_interrupts(dev)) {
			spinlock_release(&dev->__txlock);
			sched_thread_yield();
			continue;
		}

		// Enable the interrupt again
		mmio_write_uint32(imsc_addr(dev->base),
				  (mmio_read_uint32(imsc_addr(dev->base)) | UARTINT_TX));

		// Release the spinlock and wait for writability.
		//
		// As a safety net, for now, also wait for the clock to tick
		// such that we don't get a completely frozen console.
		spinlock_release(&dev->__txlock);
		uint64_t channels = SCHED_THREAD_WAIT_UART_WRITABLE | SCHED_THREAD_WAIT_TIMER;
		sched_thread_suspend(channels);
	}
}
