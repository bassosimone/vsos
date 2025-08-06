// File: kernel/tty/uart.h
// Purpose: Universal-asynchronous receiver-transmitter (UART) driver interface.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_TTY_UART_H
#define KERNEL_TTY_UART_H

#include <kernel/sys/types.h>

// Initializes the UART subsystem.
void uart_init(void);

// Attempts to read a character from the UART device.
//
// Returns either the read character or a negative errno value.
int16_t uart_try_read(void);

// Returns true when the device is readable.
bool uart_poll_read(void);

// Attempts to write the given character to the UART device.
//
// Returns 0 or a negative errno value.
int16_t uart_try_write(uint8_t ch);

// Returns true when the device is writable.
bool uart_poll_write(void);

#endif // KERNEL_TTY_UART_H
