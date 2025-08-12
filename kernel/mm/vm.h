// File: kernel/mm/vm.h
// Purpose: Virtual memory manager.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_MM_VM_H
#define KERNEL_MM_VM_H

#include <sys/param.h> // for PAGE_SIZE
#include <sys/types.h> // for uintptr_t

// Portable definitions of page mapping flags.
#define VM_MAP_FLAG_PRESENT (1 << 0)
#define VM_MAP_FLAG_WRITE (1 << 1)
#define VM_MAP_FLAG_EXEC (1 << 2)
#define VM_MAP_FLAG_USER (1 << 3)
#define VM_MAP_FLAG_DEVICE (1 << 4)

// Ensure that the page is a power of two.
static_assert(__builtin_popcount(PAGE_SIZE) == 1);

// Align a value to the value of the page below the value itself.
static inline uintptr_t vm_align_down(uintptr_t value) {
	return value & ~(PAGE_SIZE - 1);
}

// Align a value to the value of the page above the value itself.
static inline uintptr_t vm_align_up(uintptr_t value) {
	return (value + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

// Flags passed to vm_map
typedef uint64_t vm_map_flags_t;

// Initialization function that switches from physical to virtual addressing.
//
// Called by boot code when the time is ripe.
void vm_switch(void);

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

// Switches from the physical to the virtual address space.
void __vm_switch_to_virtual(uintptr_t root_table);

#endif // KERNEL_MM_VM_H
