// File: kernel/asm/asm.h
// Purpose: select arch-specific assembly routines
// SPDX-License-Identifier: MIT
#ifndef KERNEL_ASM_ASM_H
#define KERNEL_ASM_ASM_H

#ifdef ARCH_ARM64
#include <kernel/asm/arm64.h>
#endif

#endif // KERNEL_ASM_ASM_H
