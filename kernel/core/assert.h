// File: kernel/core/assert.h
// Purpose: implementation of KERNEL_ASSERT.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_CORE_ASSERT_H
#define KERNEL_CORE_ASSERT_H

#include <kernel/core/panic.h>

// Non-maskable assertion that causes panic on failure.
#define KERNEL_ASSERT(expr)                                                                                  \
	do {                                                                                                 \
		if (!(expr)) {                                                                               \
			panic("assertion failed: %s (%s:%d)", #expr, __FILE__, __LINE__);                    \
		}                                                                                            \
	} while (0)

#endif // KERNEL_CORE_ASSERT_H
