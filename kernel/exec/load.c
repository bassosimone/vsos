// File: kernel/exec/load.c
// Purpose: Load a userspace program
// SPDX-License-Identifier: MIT

#include <kernel/core/printk.h> // for printk
#include <kernel/exec/elf64.h>  // for struct elf64_image
#include <kernel/exec/layout.h> // for layout_valid_virtual_address
#include <kernel/exec/load.h>   // for load_elf64
#include <kernel/mm/page.h>     // for page_alloc
#include <kernel/mm/vm.h>       // for vm_root_pt

#include <sys/errno.h> // for ENOEXEC
#include <sys/types.h> // for int64_t

#include <string.h> // for __bzero_unaligned

static inline int64_t mmap_segment(struct vm_root_pt uroot, struct elf64_image *image, struct elf64_segment *segment) {
	// Transform flags to VM flags
	__flags32_t userflags = VM_MAP_FLAG_USER;
	if ((segment->flags & ELF64_PF_R) != 0) {
		// nothing: all pages are readable
	}
	if ((segment->flags & ELF64_PF_W) != 0) {
		userflags |= VM_MAP_FLAG_WRITE;
	}
	if ((segment->flags & ELF64_PF_X) != 0) {
		userflags |= VM_MAP_FLAG_EXEC;
	}
	printk("    user vm_map_flags: 0x%llx\n", userflags);

	// Reject loading if we aren't page aligned or outside the layout
	if (!page_aligned(segment->virt_addr)) {
		return -ENOEXEC;
	}
	if (!layout_valid_virtual_address(segment->virt_addr)) {
		return -ENOEXEC;
	}
	if (!layout_valid_virtual_address_offset(segment->virt_addr, segment->mem_size)) {
		return -ENOEXEC;
	}
	uintptr_t virt_limit = segment->virt_addr + segment->mem_size;
	printk("    virtual segment range: [0x%llx, 0x%llx)\n", segment->virt_addr, virt_limit);

	// Figure out how many bytes we need to actually allocate
	size_t alloc_bytes = vm_align_up(segment->mem_size);
	printk("    bytes to copy: %lld\n", segment->mem_size);
	printk("    aligned bytes to allocate: %lld\n", alloc_bytes);

	// Find out the number of pages we actually need
	size_t num_pages = alloc_bytes >> PAGE_SHIFT;
	printk("    pages to allocate: %lld\n", num_pages);

	// Prepare to copy into the pages
	uintptr_t virt_addr = segment->virt_addr;
	for (size_t copy_offset = 0, idx = 0; idx < num_pages; idx++) {
		// Allocate a single physical page using the allocator
		page_addr_t ppaddr = 0;
		int64_t rc = page_alloc(&ppaddr, PAGE_ALLOC_WAIT | PAGE_ALLOC_YIELD);
		if (rc != 0) {
			return rc;
		}
		KERNEL_ASSERT(ppaddr != 0);
		printk("    physical page address 0x%llx\n", ppaddr);

		// We're using identity mapping
		uintptr_t pvaddr = ppaddr;
		printk("    virtual page address 0x%llx\n", pvaddr);

		// Copy data from the ELF64 segment into the page
		uintptr_t src = (uintptr_t)image->base;
		KERNEL_ASSERT(src <= UINTPTR_MAX - segment->file_offset);
		src += segment->file_offset;
		KERNEL_ASSERT(src <= UINTPTR_MAX - copy_offset);
		src += copy_offset;
		size_t bytes_to_copy = segment->file_size - copy_offset;
		if (bytes_to_copy > PAGE_SIZE) {
			bytes_to_copy = PAGE_SIZE;
		}
		memcpy((void *)pvaddr, (void *)src, bytes_to_copy);
		printk("    copied %lld bytes into the page\n", bytes_to_copy);

		// Update the copy offset
		copy_offset += bytes_to_copy;

		// Add the page to the user page table
		printk("    user-mapping page to 0x%llx\n", virt_addr);
		vm_map_explicit(uroot, ppaddr, virt_addr, userflags | VM_MAP_FLAG_DEBUG);
		KERNEL_ASSERT(virt_addr <= UINTPTR_MAX - PAGE_SIZE);
		virt_addr += PAGE_SIZE;
	}

	return 0;
}

static int64_t allocate_stack(struct load_program *prog) {
	// Make sure our assumptions hold
	KERNEL_ASSERT(page_aligned(LAYOUT_USER_STACK_BOTTOM));
	KERNEL_ASSERT(page_aligned(LAYOUT_USER_STACK_TOP));
	KERNEL_ASSERT((LAYOUT_USER_STACK_TOP - LAYOUT_USER_STACK_BOTTOM) % PAGE_SIZE == 0);

	// Set the bottom and top to the virtual layout values
	prog->stack_bottom = LAYOUT_USER_STACK_BOTTOM;
	prog->stack_top = LAYOUT_USER_STACK_TOP;
	printk("  creating the user process stack [0x%llx, 0x%llx)\n", prog->stack_bottom, prog->stack_top);

	// Compute the number of pages to allocate
	size_t num_pages = (LAYOUT_USER_STACK_TOP - LAYOUT_USER_STACK_BOTTOM) / PAGE_SIZE;
	uintptr_t base = LAYOUT_USER_STACK_BOTTOM;
	for (size_t idx = 0; idx < num_pages; idx++) {
		// Allocate a single physical page using the allocator
		page_addr_t ppaddr = 0;
		int64_t rc = page_alloc(&ppaddr, PAGE_ALLOC_WAIT | PAGE_ALLOC_YIELD);
		if (rc != 0) {
			return rc;
		}
		KERNEL_ASSERT(ppaddr != 0);
		printk("    physical page address 0x%llx\n", ppaddr);

		// Add the page to the user page table
		printk("    user-mapping page to 0x%llx\n", base);
		vm_map_explicit(prog->root, ppaddr, base, VM_MAP_FLAG_USER | VM_MAP_FLAG_WRITE | VM_MAP_FLAG_DEBUG);
		KERNEL_ASSERT(base <= UINTPTR_MAX - PAGE_SIZE);
		base += PAGE_SIZE;
	}
	return 0;
}

int64_t load_elf64(struct load_program *prog, struct elf64_image *image) {
	// 1. ensure we're not passed null pointers
	KERNEL_ASSERT(prog != 0);
	KERNEL_ASSERT(image != 0);
	printk("load: loading ELF4 from 0x%llx\n", image);

	// 2. initialize the return argument
	__bzero_unaligned((void *)prog, sizeof(*prog));
	prog->entry = image->entry;

	// 3. create a root page table for the user process.
	int rc = page_alloc(&prog->root.table, PAGE_ALLOC_WAIT | PAGE_ALLOC_YIELD | PAGE_ALLOC_DEBUG);
	if (rc != 0) {
		return rc;
	}
	printk("  user root table 0x%llx\n", prog->root.table);

	// 4. fill the page table with kernel mappings so that system calls or traps
	// occurring in user space are able to access kernel memory.
	vm_map_kernel_memory(prog->root);
	vm_map_devices(prog->root);

	// 5. load each segment into RAM
	for (size_t idx = 0; idx < image->nsegments; idx++) {
		struct elf64_segment *segment = &image->segments[idx];
		if (segment->type != ELF64_PT_LOAD) { // we only care about PT_LOAD
			continue;
		}
		printk("  loading segment %lld\n", idx);
		int64_t rc = mmap_segment(prog->root, image, segment);
		if (rc != 0) {
			return rc;
		}
	}

	// Finally allocate the user stack
	return allocate_stack(prog);
}
