// File: kernel/tty/uart.h
// Purpose: UART driver.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_TTY_UART_H
#define KERNEL_TTY_UART_H

#include <kernel/mm/vm.h> // for struct vm_root_pt

#include <sys/cdefs.h> // for __BEGIN_DECLS
#include <sys/types.h> // for size_t

__BEGIN_DECLS

// Early initialization for the UART driver.
//
// Called by the boot sybsystem.
void uart_init_early(void) __NOEXCEPT;

// Initialize memory mapping for the UART driver.
//
// Called by the mm subsystem.
void uart_init_mm(struct vm_root_pt root) __NOEXCEPT;

// Initialize IRQs for the UART driver.
//
// Called by the irq subsystem.
void uart_init_irqs(void) __NOEXCEPT;

// Handle UART interrupt request.
//
// Called by the irq handler.
void uart_isr(void) __NOEXCEPT;

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
ssize_t uart_recv(char *buf, size_t count, __flags32_t flags) __NOEXCEPT;

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
ssize_t uart_send(const char *buf, size_t count, __flags32_t flags) __NOEXCEPT;

__END_DECLS

#endif // KERNEL_TTY_UART_H
