// File: kernel/tty/uart_arm64.c
// Purpose: Universal-asynchronous receiver-transmitter (UART) driver for ARM64.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/asm/arm64.h>
#include <kernel/sys/errno.h>
#include <kernel/tty/uart.h>

// PL011 UART base address in QEMU virt
#define UART0_BASE 0x09000000

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

void uart_init(void) {
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
}

int16_t uart_try_read(void) {
	if (!uart_poll_read()) {
		return -EAGAIN;
	}
	dsb_sy(); // Ensure the subsequent read "happens after"
	return (int16_t)(UARTDR & 0xFF);
}

bool uart_poll_read(void) {
	return (UARTFR & UARTFR_RXFE) == 0;
}

int16_t uart_try_write(uint8_t ch) {
	if (!uart_poll_write()) {
		return -EAGAIN;
	}
	dsb_sy(); // Ensure the subsequent write "happens after"
	UARTDR = ch;
	return 0;
}

bool uart_poll_write(void) {
	return (UARTFR & UARTFR_TXFF) == 0;
}
