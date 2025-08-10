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

// Attempts to read a character from the UART device.
//
// This function is a cooperative synchronization point. The code will
// possibly yield the CPU if we need rescheduling.
//
// Returns either the read character or a negative errno value.
int16_t uart_try_read(void);

// Returns true when the device is readable.
bool uart_readable(void);

// Attempts to write the given character to the UART device.
//
// This function is a cooperative synchronization point. The code will
// possibly yield the CPU if we need rescheduling.
//
// Returns 0 or a negative errno value.
int16_t uart_try_write(uint8_t ch);

// Like uart_try_write but without the cooperative synchronization
// point. We MUST use this function inside printk since printk
// MUST NOT be preempted (otherwise we get nonsense output).
int64_t __uart_try_write(uint8_t ch);

// Returns true when the device is writable.
bool uart_writable(void);

// The read system call using the UART.
ssize_t uart_read(char *buffer, size_t siz);

// MD implementation of reading a byte from the UART.
//
// This function should only be called by functions in this subsystem.
//
// Returns a >= char (uint8_t) or a negative errno value.
int16_t __uart_getchar(uint32_t flags);

// The write system call using the UART.
ssize_t uart_write(const char *buffer, size_t siz);

#endif // KERNEL_TTY_UART_H
