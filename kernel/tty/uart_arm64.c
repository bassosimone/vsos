// File: kernel/tty/uart_arm64.c
// Purpose: Universal-asynchronous receiver-transmitter (UART) driver for ARM64.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/drivers/pl011.h> // for struct pl011_device
#include <kernel/tty/uart.h>	  // for uart_init_early

#include <sys/types.h> // for size_t

// PL011 UART base address in QEMU virt
#define UART0_BASE 0x09000000

static struct pl011_device uart0;

void uart_init_early(void) {
	pl011_init_struct(&uart0, UART0_BASE, "uart0");
	pl011_init_early(&uart0);
}

void uart_init_mm(void) {
	pl011_init_mm(&uart0);
}

void uart_init_irq(void) {
	pl011_init_irqs(&uart0);
}

void uart_irq(void) {
	pl011_isr(&uart0);
}

ssize_t uart_send(const char *buf, size_t count, uint32_t flags) {
	return pl011_send(&uart0, buf, count, flags);
}

ssize_t uart_recv(char *buf, size_t count, uint32_t flags) {
	return pl011_recv(&uart0, buf, count, flags);
}
