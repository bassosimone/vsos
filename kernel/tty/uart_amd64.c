// File: kernel/tty/uart_amd64.c
// Purpose: Universal-asynchronous receiver-transmitter (UART) driver for AMD64.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/asm/amd64.h>
#include <kernel/core/assert.h>
#include <kernel/errno.h>
#include <kernel/sys/types.h>
#include <kernel/tty/uart.h>

// The base address of the COM1 serial port.
#define COM1 0x3F8

// R/W register.
#define UART_DATA 0

// IER: Interrupt Enable Register.
#define UART_IER 1

// LSR: Line Status Register.
#define UART_LSR 5

// DR: Data Ready.
#define UART_LSR_DR 0x01

// THRE: Transmit Holding Register Empty.
#define UART_LSR_THRE 0x20

void uart_init(void) {
	outb(COM1 + UART_IER, 0x00); // Disable all UART interrupts
}

int16_t uart_try_read(void) {
	if (!uart_poll_read()) {
		return -EAGAIN;
	}
	return inb(COM1 + UART_DATA);
}

bool uart_poll_read(void) { return (inb(COM1 + UART_LSR) & UART_LSR_DR) != 0; }

int16_t uart_try_write(uint8_t ch) {
	if (!uart_poll_write()) {
		return -EAGAIN;
	}
	outb(COM1 + UART_DATA, ch);
	return 0;
}

bool uart_poll_write(void) { return (inb(COM1 + UART_LSR) & UART_LSR_THRE) != 0; }
