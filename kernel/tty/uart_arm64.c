// File: kernel/tty/uart_arm64.c
// Purpose: Universal-asynchronous receiver-transmitter (UART) driver for ARM64.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/asm/arm64.h>
#include <kernel/core/printk.h>
#include <kernel/mm/vmap.h>
#include <kernel/sched/sched.h>
#include <kernel/sys/errno.h>
#include <kernel/tty/uart.h>

// PL011 UART base address in QEMU virt
#define UART0_BASE 0x09000000

// Limit of the memory mapped region for the UART
#define UART0_LIMIT UART0_BASE + 0x1000ULL

// Data register
#define UARTDR (*(volatile uint32_t *)(UART0_BASE + 0x00))

// Flags register
#define UARTFR (*(volatile uint32_t *)(UART0_BASE + 0x18))

// Control register: enable UART, TX/RX
#define UARTCR 0x30

// Interrupt mask set/clear register
#define UARTIMSC 0x38

// Interrupt clear (i.e., acknowledge) register
#define UARTICR 0x44

// Transmit FIFO full
#define UARTFR_TXFF (1 << 5)

// Receive FIFO empty
#define UARTFR_RXFE (1 << 4)

void uart_init_early(void) {
	// QEMU's PL011 is pre-initialized.
	//
	// However, let's do the exercise of resetting it.
	volatile uint32_t *cr = (volatile uint32_t *)(UART0_BASE + UARTCR);
	volatile uint32_t *imsc = (volatile uint32_t *)(UART0_BASE + UARTIMSC);
	volatile uint32_t *icr = (volatile uint32_t *)(UART0_BASE + UARTICR);

	// Disable UART and "push" changes
	*cr = 0;
	dsb_sy();

	// Mask all IRQs
	*imsc = 0x0;

	// Clear any pending IRQs
	*icr = 0x7FF;

	// "Push" changes
	dsb_sy();

	// Enable TXE | RXE | UARTEN and "push" changes
	*cr = (1 << 9) | (1 << 8) | (1 << 0);
	dsb_sy();

	// This is the first moment in the boot process where
	// the printk call would actually work
	printk("uart0: enabled: TXE | RXE | UARTEN\n");
}

void uart_init_mm(void) {
	printk("uart0: mmap_identity %llx - %llx\n", UART0_BASE, UART0_LIMIT);
	mmap_identity(UART0_BASE, UART0_LIMIT, MM_FLAG_DEVICE | MM_FLAG_WRITE);
}

void uart_init_irq(void) {
	// TODO(bassosimone): implement this function
}

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
