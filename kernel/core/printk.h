// File: kernel/core/printk.h
// Purpose: define the printk function.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_CORE_PRINTK_H
#define KERNEL_CORE_PRINTK_H

#include <sys/cdefs.h> // for __BEGIN_DECLS

__BEGIN_DECLS

// Formats the arguments and print the output on the serial console.
void printk(const char *format, ...) __NOEXCEPT;

// Like printk but with already parsed variable argument pointer.
void vprintk(const char *fmt, __builtin_va_list ap) __NOEXCEPT;

__END_DECLS

#endif // KERNEL_CORE_PRINTK_H
