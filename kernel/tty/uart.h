// File: kernel/tty/uart.h
// Purpose: Universal-asynchronous receiver-transmitter (UART) driver interface.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_TTY_UART_H
#define KERNEL_TTY_UART_H

#include <kernel/sys/types.h>

// Early initialization for the UART driver.
//
// Called by the boot sybsystem.
void uart_init_early(void);

// Initialize memory mapping for the UART driver.
//
// Called by the mm subsystem.
void uart_init_mm(void);

// Initialize IRQs for the UART driver.
//
// Called by the irq subsystem.
void uart_init_irq(void);

// Handle UART interrupt request.
//
// Called by the irq handler.
void uart_irq(void);

// The read system call using the UART.
ssize_t uart_read(char *buffer, size_t siz);

// Allows reading bytes from the UART.
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
// This function blocks until interrupts are enabled.
ssize_t uart_recv(char *buf, size_t count, uint32_t flags);

// The write system call using the UART.
ssize_t uart_write(const char *buffer, size_t siz);

// Allows writing bytes to the UART.
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
// we have not enabled interrupts yet.
ssize_t uart_send(const char *buf, size_t count, uint32_t flags);

#endif // KERNEL_TTY_UART_H
