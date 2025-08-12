// File: kernel/mm/vm.h
// Purpose: Virtual memory manager.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_MM_VM_H
#define KERNEL_MM_VM_H

#include <sys/types.h> // for uintptr_t

// Returns the kernel-internal direct mapping view of addr.
//
// The parameter is a physical address.
//
// The return value is a virtual address.
//
// Before intializing the VM, the direct mapping is the identity mapping.
//
// Afterwards, there is a fixed offset between virtual and physical.
uintptr_t /* virt_addr */ __vm_direct_map(uintptr_t phys_addr);

// Install the memory map for the kernel memory inside the root table.
void __vm_map_kernel_memory(uintptr_t root_table);

// Requests the devices to install their memory map.
void __vm_map_devices(uintptr_t root_table);

#endif // KERNEL_MM_VM_H
