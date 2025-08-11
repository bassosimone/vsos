// File: kernel/tty/uart_arm64.c
// Purpose: Universal-asynchronous receiver-transmitter (UART) driver for ARM64.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/asm/asm.h>
#include <kernel/core/assert.h>
#include <kernel/core/printk.h>
#include <kernel/core/ringbuf.h>
#include <kernel/core/spinlock.h>
#include <kernel/mm/mm.h>
#include <kernel/sched/sched.h>
#include <kernel/tty/uart.h>

#include <sys/errno.h>
#include <sys/fcntl.h>

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

// Initialize the UART living at the given base.
static inline void uart_init_early_base(const char *device, uintptr_t base) {
	// Disable UART and "push" changes
	mmio_write_uint32(cr_addr(base), 0);

	// Mask all IRQs
	mmio_write_uint32(imsc_addr(base), 0x0);

	// Clear any pending IRQs
	mmio_write_uint32(icr_addr(base), UARTICR_CLR_ALL);

	// Enable the device, receiving, and sending.
	mmio_write_uint32(cr_addr(base), UARTCR_UARTEN | UARTCR_RXE | UARTCR_TXE);

	// Let the user know what we did.
	printk("%s: UARTCR |= UARTEN | RXE | TXE\n", device);
}

static inline void uart_init_mm_base(const char *device, uintptr_t base) {
	uintptr_t limit = memory_limit(base);
	printk("%s: mmap_identity %llx - %llx\n", device, base, limit);
	mmap_identity(base, limit, MM_FLAG_DEVICE | MM_FLAG_WRITE);
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

// Change our behavior once we have setup interrupts.
static volatile uint64_t __has_interrupts = 0;

static inline void set_has_interrupts(void) {
	__atomic_store_n(&__has_interrupts, 1, __ATOMIC_RELEASE);
}

static inline bool has_enabled_interrupts(void) {
	return __atomic_load_n(&__has_interrupts, __ATOMIC_ACQUIRE) != 0;
}

static inline void uart_init_irq_base(const char *device, uintptr_t base) {
	// Enable the FIFO behavior
	mmio_write_uint32(clr_h_addr(base), (mmio_read_uint32(clr_h_addr(base)) | UARTLCR_H_FEN));

	// Trigger interrupts when the RX level is 1/8 and the TX level is 1/8
	mmio_write_uint32(ifls_addr(base), 0);

	// Defensively clear all potentially pending interrupts.
	mmio_write_uint32(icr_addr(base), UARTICR_CLR_ALL);

	// Select the events to receive notifications about.
	mmio_write_uint32(imsc_addr(base), UARTINT_RX | UARTINT_RT | UARTINT_OE);

	// Let the user know what we did *before* turning interrupts on.
	printk("%s: UARTIMSC |= RX | RT | OE\n", device);

	// Ensure we all know we have interrupts.
	set_has_interrupts();
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
static inline bool __uart_readable(uintptr_t base) {
	return (mmio_read_uint32(fr_addr(base)) & UARTFR_RXFE) == 0;
}

// Return whether the UART is writable.
static inline bool __uart_writable(uintptr_t base) {
	return (mmio_read_uint32(fr_addr(base)) & UARTFR_TXFF) == 0;
}

// UARTMIS: masked interrupt status register.
static inline volatile uint32_t *mis_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x40);
}

// Ring buffer for receiving characters from the UART.
static struct ringbuf rxbuf;

// Service the IRQ for the given base UART addr.
static inline void __uart_irq_base(uintptr_t base) {
	uint32_t mis = mmio_read_uint32(mis_addr(base));

	// Handle the case of the UART being readable
	if ((mis & (UARTINT_RX | UARTINT_RT | UARTINT_OE)) != 0) {
		// Drain the RX FIFO first including per-byte flags
		while (__uart_readable(base)) {
			uint16_t value = (uint16_t)(mmio_read_uint32(dr_addr(base)) & 0x0FFF);
			(void)ringbuf_push(&rxbuf, value);
		}

		// Clear RX-related causes and stop the interrupt from firing
		uint32_t mask = UARTINT_RX | UARTINT_RT | UARTINT_FE | UARTINT_PE | UARTINT_BE | UARTINT_OE;
		mmio_write_uint32(icr_addr(base), mask);

		// Wake up any sleeping thread
		sched_thread_resume_all(SCHED_THREAD_WAIT_UART_READABLE);
	}

	// Handle the case of the UART being writable.
	if ((mis & UARTINT_TX) != 0) {
		// Acknowledge the TX interrupt
		mmio_write_uint32(icr_addr(base), UARTINT_TX);

		// Mask the interrupt to avoid level-triggered interrupt storms.
		mmio_write_uint32(imsc_addr(base), (mmio_read_uint32(imsc_addr(base)) & ~UARTINT_TX));

		// Let user space know it can send more
		sched_thread_resume_all(SCHED_THREAD_WAIT_UART_WRITABLE);
	}
}

// Spinlock protecting the receive path.
static struct spinlock rxlock;

ssize_t uart_recv(char *buf, size_t count, uint32_t flags) {
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
		while (spinlock_try_acquire(&rxlock) != 0) {
			if ((flags & O_NONBLOCK) != 0) {
				return (off <= 0) ? -EAGAIN : (ssize_t)off;
			}
			sched_thread_yield();
		}

		// Attempt to read from the ring buffer
		uint16_t data = 0;
		bool success = ringbuf_pop(&rxbuf, &data);

		// Drop the spinlock now that we've accessed the buffer
		spinlock_release(&rxlock);

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

// Spinlock protecting the send path.
static struct spinlock txlock;

static inline ssize_t __uart_send(uintptr_t base, const char *buf, size_t count, uint32_t flags) {
	// Ensure we're not going to overflow the return value
	count = (count <= SSIZE_MAX) ? count : SSIZE_MAX;

	// Count the number of characters sent
	size_t tot = 0;

	for (;;) {
		// Grab the lock granting us the permission to transmit,
		// however, be careful with O_NONBLOCK users.
		while (spinlock_try_acquire(&txlock) != 0) {
			if ((flags & O_NONBLOCK) != 0) {
				return (tot <= 0) ? -EAGAIN : (ssize_t)tot;
			}
			sched_thread_yield();
		}

		// Awesome, now send as much as possible until the
		// device tells us that it has enough data
		while (__uart_writable(base) && tot < count) {
			mmio_write_uint32(dr_addr(base), (uint32_t)((uint8_t)buf[tot++]));
		}

		// If everything has been sent, our job is done
		if (tot >= count) {
			spinlock_release(&txlock);
			return (ssize_t)tot;
		}

		// If we are nonblocking, this is the time where we'd block
		if ((flags & O_NONBLOCK) != 0) {
			spinlock_release(&txlock);
			return (tot <= 0) ? -EAGAIN : (ssize_t)tot;
		}

		// If we are without interrupts and we are allowed to
		// block, the best we can do is yield the CPU.
		if (!has_enabled_interrupts()) {
			spinlock_release(&txlock);
			sched_thread_yield();
			continue;
		}

		// Enable the interrupt again
		mmio_write_uint32(imsc_addr(base), (mmio_read_uint32(imsc_addr(base)) | UARTINT_TX));

		// Release the spinlock and wait for writability.
		//
		// As a safety net, for now, also wait for the clock to tick
		// such that we don't get a completely frozen console.
		spinlock_release(&txlock);
		uint64_t channels = SCHED_THREAD_WAIT_UART_WRITABLE | SCHED_THREAD_WAIT_TIMER;
		sched_thread_suspend(channels);
	}
}

// PL011 UART base address in QEMU virt
#define UART0_BASE 0x09000000

void uart_init_early(void) {
	uart_init_early_base("uart0", UART0_BASE);
}

void uart_init_mm(void) {
	uart_init_mm_base("uart0", UART0_BASE);
}

void uart_init_irq(void) {
	uart_init_irq_base("uart0", UART0_BASE);
}

void uart_irq(void) {
	__uart_irq_base(UART0_BASE);
}

ssize_t uart_send(const char *buf, size_t count, uint32_t flags) {
	return __uart_send(UART0_BASE, buf, count, flags);
}
