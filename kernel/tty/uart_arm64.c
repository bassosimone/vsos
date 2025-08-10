// File: kernel/tty/uart_arm64.c
// Purpose: Universal-asynchronous receiver-transmitter (UART) driver for ARM64.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/asm/arm64.h>
#include <kernel/core/assert.h>
#include <kernel/core/printk.h>
#include <kernel/core/ringbuf.h>
#include <kernel/mm/vmap.h>
#include <kernel/sched/sched.h>
#include <kernel/sys/errno.h>
#include <kernel/sys/fcntl.h>
#include <kernel/tty/uart.h>

// PL011 UART base address in QEMU virt
#define UART0_BASE 0x09000000

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
	*cr_addr(base) = 0;
	dsb_sy();

	// Mask all IRQs
	*imsc_addr(base) = 0x0;

	// Clear any pending IRQs
	*icr_addr(base) = UARTICR_CLR_ALL;

	// "Push" changes
	dsb_sy();

	// Enable the device, receiving, and sending.
	*cr_addr(base) = UARTCR_UARTEN | UARTCR_RXE | UARTCR_TXE;
	dsb_sy();

	// Let the user know what we did.
	printk("%s: UARTCR |= UARTEN | RXE | TXE\n", device);
}

void uart_init_early(void) {
	uart_init_early_base("uart0", UART0_BASE);
}

static inline void uart_init_mm_base(const char *device, uintptr_t base) {
	uintptr_t limit = memory_limit(base);
	printk("%s: mmap_identity %llx - %llx\n", device, base, limit);
	mmap_identity(base, limit, MM_FLAG_DEVICE | MM_FLAG_WRITE);
}

void uart_init_mm(void) {
	uart_init_mm_base("uart0", UART0_BASE);
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

static inline void uart_init_irq_base(const char *device, uintptr_t base) {
	// Enable the FIFO behavior
	*clr_h_addr(base) |= UARTLCR_H_FEN;

	// Trigger interrupts when the RX level is 1/8 and the TX level is 1/8
	*ifls_addr(base) = 0;

	// Defensively clear all potentially pending interrupts.
	*icr_addr(base) = UARTICR_CLR_ALL;

	// Select the events to receive notifications about.
	*imsc_addr(base) = UARTINT_RX | UARTINT_RT | UARTINT_OE;

	// Let the user know what we did *before* turning interrupts on.
	printk("%s: UARTIMSC |= RX | OE\n", device);

	// Publish our changes.
	dsb_sy();
}

void uart_init_irq(void) {
	uart_init_irq_base("uart0", UART0_BASE);
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
	return (*fr_addr(base) & UARTFR_RXFE) == 0;
}

// UARTMIS: masked interrupt status register.
static inline volatile uint32_t *mis_addr(uintptr_t base) {
	return (volatile uint32_t *)(base + 0x40);
}

// Ring buffer for receiving characters from the UART.
static struct ringbuf rxbuf;

// Service the IRQ for the given base UART addr.
static inline void __uart_irq_base(uintptr_t base) {
	uint32_t mis = *mis_addr(base);

	// Handle the case of the UART being readable
	if ((mis & (UARTINT_RX | UARTINT_RT | UARTINT_OE)) != 0) {
		// Drain the RX FIFO first including per-byte flags
		while (__uart_readable(base)) {
			(void)ringbuf_push(&rxbuf, *dr_addr(base) & 0xFFFF);
		}

		// Clear RX-related causes and stop the interrupt from firing
		*icr_addr(base) = UARTINT_RX | UARTINT_RT | UARTINT_FE | UARTINT_PE | UARTINT_BE | UARTINT_OE;

		// Wake up any sleeping thread
		sched_thread_resume_all(SCHED_THREAD_WAIT_UART_READABLE);
	}
}

void uart_irq(void) {
	__uart_irq_base(UART0_BASE);
}

int16_t __uart_getchar(uint32_t flags) {
	for (;;) {
		// First attempt to read from the ring buffer
		uint16_t data = 0;
		if (ringbuf_pop(&rxbuf, &data)) {
			// Separate actual data from error flags.
			uint8_t err = (data & 0x0F00) >> 8;
			if (err != 0) {
				return -EIO;
			}
			return data & 0xFF;
		}

		// Handle the case of nonblocking reading
		if ((flags & O_NONBLOCK) != 0) {
			return -EAGAIN;
		}

		// Suspend until we have data to read
		sched_thread_suspend(SCHED_THREAD_WAIT_UART_READABLE);
	}
}

// TODO(bassosimone): the code below should be refactored
// and completely deprecated. We want to only use IRQs.

// Data register
#define UARTDR (*(volatile uint32_t *)(UART0_BASE + 0x00))

// Flags register
#define UARTFR (*(volatile uint32_t *)(UART0_BASE + 0x18))

int16_t uart_try_read(void) {
	sched_thread_maybe_yield();
	if (!uart_readable()) {
		return -EAGAIN;
	}
	dsb_sy(); // Ensure the subsequent read "happens after"
	return (int16_t)(UARTDR & 0xFF);
}

bool uart_readable(void) {
	return (UARTFR & UARTFR_RXFE) == 0;
}

int16_t uart_try_write(uint8_t ch) {
	sched_thread_maybe_yield();
	return __uart_try_write(ch);
}

int64_t __uart_try_write(uint8_t ch) {
	if (!uart_writable()) {
		return -EAGAIN;
	}
	dsb_sy(); // Ensure the subsequent write "happens after"
	UARTDR = ch;
	return 0;
}

bool uart_writable(void) {
	return (UARTFR & UARTFR_TXFF) == 0;
}
