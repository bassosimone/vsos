// File: kernel/asm/amd64.h
// Purpose: AMD64 assembly routines
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#ifndef KERNEL_ASM_AMD64_H
#define KERNEL_ASM_AMD64_H

#include <kernel/sys/types.h>

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
	__asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void hlt(void) { __asm__ volatile("hlt"); }

#endif // KERNEL_ASM_AMD64
