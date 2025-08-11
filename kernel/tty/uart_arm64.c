// File: kernel/tty/uart_arm64.c
// Purpose: UART driver for ARM64 devices.
// SPDX-License-Identifier: MIT

#include <kernel/drivers/pl011_arm64.h> // for struct pl011_device
#include <kernel/tty/uart.h>		// for uart_init_early

#include <sys/types.h> // for size_t

// PL011 UART base address in QEMU virt
#define PL011_MMIO_BASE 0x09000000

static struct pl011_device uart0;

void uart_init_early(void) {
	pl011_init_struct(&uart0, PL011_MMIO_BASE, "uart0");
	pl011_init_early(&uart0);
}

void uart_init_mm(void) {
	pl011_init_mm(&uart0);
}

void uart_init_irqs(void) {
	pl011_init_irqs(&uart0);
}

void uart_isr(void) {
	pl011_isr(&uart0);
}

ssize_t uart_send(const char *buf, size_t count, uint32_t flags) {
	return pl011_send(&uart0, buf, count, flags);
}

ssize_t uart_recv(char *buf, size_t count, uint32_t flags) {
	return pl011_recv(&uart0, buf, count, flags);
}
