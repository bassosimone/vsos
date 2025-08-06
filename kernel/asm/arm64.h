// File: kernel/asm/arm64.h
// Purpose: ARM64 assembly routines
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#ifndef KERNEL_ASM_ARM64_H
#define KERNEL_ASM_ARM64_H

#include <kernel/sys/types.h>

static inline void wfi(void) { __asm__ volatile("wfi"); }

#endif // KERNEL_ASM_ARM64
