// File: kernel/mm/vm.c
// Purpose: Virtual memory manager.
// SPDX-License-Identifier: MIT

#include <kernel/mm/vm.h> // for __vm_direct_map

#include <sys/types.h> // for uintptr_t

uintptr_t __vm_direct_map(uintptr_t phys_addr) {
	// TODO(bassosimone): implement direct mapping
	return phys_addr;
}
