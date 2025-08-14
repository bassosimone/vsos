// File: kernel/exec/elf64.h
// Purpose: Code for parsing ELF64 binaries.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_EXEC_ELF64_H
#define KERNEL_EXEC_ELF64_H

#include <sys/types.h> // for size_t

// Segment permission flags
#define ELF64_PF_X 1
#define ELF64_PF_W 2
#define ELF64_PF_R 4

// Segment type
#define ELF64_PT_NULL 0
#define ELF64_PT_LOAD 1
#define ELF64_PT_DYNAMIC 2
#define ELF64_PT_INTERP 3
#define ELF64_PT_NOTE 4

// Segment within an ELF64 binary.
struct elf64_segment {
	// Virtual address at which to load this section.
	uintptr_t virt_addr;

	// How much virtual memory is required for this section.
	size_t mem_size;

	// Where in the file we should copy from.
	uintptr_t file_offset;

	// How much to copy from the file.
	size_t file_size;

	// ELF permission flags
	uint32_t flags;

	// Segment type.
	uint32_t type;
};

// Maximum number of segments we can load with this library.
#define ELF64_MAX_SEGMENTS 16

// Memory representation of the ELF64 binary.
struct elf64_image {
	// The borrowed raw memory address where the binary is located.
	const void *base;

	// The total size of the binary.
	size_t size;

	// The address at which userspace needs to jump.
	uintptr_t entry;

	// Space for segments.
	struct elf64_segment segments[ELF64_MAX_SEGMENTS];

	// Number of segments.
	size_t nsegments;
};

// Fills image with a simplified view of the ELF64 binary at [data, data+size).
//
// Returns 0 on success and a negative errno value on failure.
int64_t elf64_parse(struct elf64_image *image, const void *data, size_t size);

#endif // KERNEL_EXEC_ELF64_H
