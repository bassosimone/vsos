// File: kernel/mm/vm.h
// Purpose: Virtual memory manager.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_MM_VM_H
#define KERNEL_MM_VM_H

#include <kernel/mm/page.h> // for page_addr_t

#include <sys/param.h> // for PAGE_SIZE
#include <sys/types.h> // for uintptr_t

// Flag indicating that a page is present.
#define VM_MAP_FLAG_PRESENT (1 << 0)

// Flag indicating that a page is writable.
#define VM_MAP_FLAG_WRITE (1 << 1)

// Flag indicating that a page is executable.
#define VM_MAP_FLAG_EXEC (1 << 2)

// Flag indicating that a page is owned by userspace.
#define VM_MAP_FLAG_USER (1 << 3)

// Flag indicating a page used for MMIO.
#define VM_MAP_FLAG_DEVICE (1 << 4)

// Flag indicating that we should print every low-level operation
// occurring while allocating a page.
#define VM_MAP_FLAG_DEBUG (1 << 5)

// Ensure that the page is a power of two.
static_assert(__builtin_popcount(PAGE_SIZE) == 1);

// Align a value to the value of the page below the value itself.
static inline uintptr_t vm_align_down(uintptr_t value) {
	return value & ~PAGE_OFFSET_MASK;
}

// Align a value to the value of the page above the value itself.
static inline uintptr_t vm_align_up(uintptr_t value) {
	KERNEL_ASSERT(value <= UINTPTR_MAX - PAGE_OFFSET_MASK);
	return (value + PAGE_OFFSET_MASK) & ~PAGE_OFFSET_MASK;
}

// Type representing the root page table.
struct vm_root_pt {
	uintptr_t table;
};

// Initialization function that switches from physical to virtual addressing.
//
// We are forced to use identity mapping since the linker script uses a fixed address.
//
// Address won't change after this function, but the MMU will be online.
//
// Called by boot code when the time is ripe.
void vm_switch(void);

// Accessor function that returns the kernel root PT.
struct vm_root_pt vm_kernel_root_pt(void);

// Maps a memory [start, end) memory range using the given flags.
//
// The root argument must be the root page table.
//
// The start_addr argument must be aligned on a page boundary or we'll panic.
//
// The end_addr argument is automatically aligned up to the next page.
//
// We are using identity mapping so the addresses won't change.
void vm_map_range_identity(struct vm_root_pt root, page_addr_t start, page_addr_t end, __flags32_t flags);

// Explicitly sets a VM mapping between paddr and vaddr using the root and flags.
//
// This is the proper API to use when your objective is to map user pages or to remap
// specific areas of the memory for kernel usage with different flags.
void vm_map_explicit(struct vm_root_pt root, page_addr_t paddr, uintptr_t vaddr, __flags32_t flags);

// Maps the kernel memory into the given root table.
//
// When you're creating the memory layout for a new process, you need to
// call this function to allow the process to handle traps.
//
// So, the process basically gets a copy of the kernel layout and the
// kernel layout is immutable after the boot.
void vm_map_kernel_memory(struct vm_root_pt root);

// Like vm_map_kernel_memory but for mapping devices memory.
void vm_map_devices(struct vm_root_pt root);

// Convenience wrapper for vm_map_explicit to setup identity mapping for paddr.
static inline void vm_map_identity(struct vm_root_pt root, page_addr_t paddr, __flags32_t flags) {
	return vm_map_explicit(root, paddr, paddr, flags);
}

// Given the user root page table and a user vaddr, map it back to a paddr.
//
// Use the flags to request for debugging.
//
// Returns 0 on success and -EINVAL on failure.
//
// Panics if paddr is 0.
//
// Clears paddr to 0 in case of error.
__status_t vm_user_virt_to_phys(uintptr_t *paddr, struct vm_root_pt root, uintptr_t vaddr, __flags32_t flags);

// Internal machine dependent mapping implementation that assumes that
// we have already checked that arguments are correctly aligned.
//
// Prefer __vm_map_explicit to calling this function.
void __vm_map_explicit_assume_aligned(struct vm_root_pt root, page_addr_t paddr, uintptr_t vaddr, __flags32_t flags);

// Internal machine-dependent function for using the MMU.
//
// Should only be called within this subsystem.
void __vm_switch(struct vm_root_pt pt);

// This variable contains the kernel root page table.
//
// In C code, use the vm_kernel_root_pt accessor instead.
//
// In assembly, well, you know what to do.
extern uintptr_t __vm_kernel_root_pt;

#endif // KERNEL_MM_VM_H
