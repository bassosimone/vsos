// File: kernel/mm/vmap.c
// Purpose: Virtual-memory-mapper implementation.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines

#include <kernel/core/assert.h> // for KERNEL_ASSERT
#include <kernel/core/printk.h> // for printk

#include <kernel/mm/mm.h> // for mm_map_identity, etc.

// TODO(bassosimone): replace this API
void mm_map_identity(struct vm_root_pt root, page_addr_t start, uintptr_t end, vm_map_flags_t flags) {
	int rc = vm_map(root, start, end, flags, 0);
	KERNEL_ASSERT(rc == 0);
}
