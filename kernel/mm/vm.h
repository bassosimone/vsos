// File: kernel/mm/vm.h
// Purpose: Virtual memory manager.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_MM_VM_H
#define KERNEL_MM_VM_H

#include <kernel/mm/page.h> // for page_addr_t

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

// Type representing the root page table.
struct vm_root_pt {
	uintptr_t table;
};

// Flags passed to vm_map
typedef uint64_t vm_map_flags_t;

// Initialization function that switches from physical to virtual addressing.
//
// Called by boot code when the time is ripe.
void vm_switch(void);

// Remaps the range of physical addresses starting at start and ending at end to remapped.
//
// The root argument must be the root page table.
//
// The start argument must be aligned on a page boundary or we'll panic.
//
// The end argument is automatically aligned up to the next page.
//
// The remapped argument may be 0. Otherwise we store in there the remapped address.
//
// Returns 0 on success and a negative errno value otherwise.
int64_t
vm_map(struct vm_root_pt root, page_addr_t start, uintptr_t end, vm_map_flags_t flags, page_addr_t *remapped);

// Explicitly sets a VM mapping between paddr and vaddr using the root page table and flags.
//
// Consumers should generally use higher level APIs such as vm_map.
void __vm_map_explicit(struct vm_root_pt root, page_addr_t paddr, uintptr_t vaddr, vm_map_flags_t flags);

// Internal machine dependent mapping implementation that assumes that
// we have already checked that arguments are correctly aligned.
//
// Prefer __vm_map_explicit to calling this function.
void __vm_map_explicit_assume_aligned(struct vm_root_pt root,
                                      page_addr_t paddr,
                                      uintptr_t vaddr,
                                      vm_map_flags_t flags);

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

// Switches from the physical to the virtual address space.
void __vm_switch_to_virtual(struct vm_root_pt pt);

#endif // KERNEL_MM_VM_H
