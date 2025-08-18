// File: kernel/syscall/io.h
// Purpose: copy_from_user and copy_to_user
// SPDX-License-Identifier: MIT

#include <kernel/mm/page.h>     // for PAGE_OFFSET_MASK
#include <kernel/mm/vm.h>       // for vm_user_virt_to_phys
#include <kernel/sched/sched.h> // for sched_current_process_page_table
#include <kernel/syscall/io.h>  // for copy_from_user

#include <sys/types.h> // for ssize_t

#include <string.h> // for memcpy

ssize_t copy_from_user(char *dst, const char *src, size_t count) {
	// Ensure we don't overflow the return value
	if (count > SSIZE_MAX) {
		count = SSIZE_MAX;
	}

	// Get the page table used by the current process
	struct vm_root_pt table = {0};
	__status_t rc = sched_current_process_page_table(&table);
	if (rc != 0) {
		return rc;
	}

	// Continue until we copied as much as possible
	for (size_t offset = 0; offset < count;) {
		// Map the virtual address to a physical address
		uintptr_t phys_addr;
		rc = vm_user_virt_to_phys(&phys_addr, table, (uintptr_t)src + offset, 0);
		if (rc != 0) {
			return (ssize_t)offset; // Return bytes copied so far
		}

		// Calculate how many bytes we can copy from this page
		size_t page_offset = phys_addr & PAGE_OFFSET_MASK;
		size_t available_in_page = PAGE_SIZE - page_offset;
		size_t remaining = count - offset;
		size_t to_copy = (available_in_page < remaining) ? available_in_page : remaining;

		// Perform the copy and advance
		memcpy(dst + offset, (const char *)phys_addr, to_copy);
		offset += to_copy;
	}

	return (ssize_t)count; // Successfully copied all bytes
}

ssize_t copy_to_user(char *dst, const char *src, size_t count) {
	// Ensure we don't overflow the return value
	if (count > SSIZE_MAX) {
		count = SSIZE_MAX;
	}

	// Get the page table used by the current process
	struct vm_root_pt table = {0};
	int64_t rc = sched_current_process_page_table(&table);
	if (rc != 0) {
		return rc;
	}

	// Continue until we copied as much as possible
	for (size_t offset = 0; offset < count;) {
		// Map the virtual address to a physical address
		uintptr_t phys_addr;
		rc = vm_user_virt_to_phys(&phys_addr, table, (uintptr_t)dst + offset, 0);
		if (rc != 0) {
			return (ssize_t)offset; // Return bytes copied so far
		}

		// Calculate how many bytes we can copy to this page
		size_t page_offset = phys_addr & PAGE_OFFSET_MASK;
		size_t available_in_page = PAGE_SIZE - page_offset;
		size_t remaining = count - offset;
		size_t to_copy = (available_in_page < remaining) ? available_in_page : remaining;

		// Perform the copy and advance
		memcpy((char *)phys_addr, src + offset, to_copy);
		offset += to_copy;
	}

	return (ssize_t)count; // Successfully copied all bytes
}
