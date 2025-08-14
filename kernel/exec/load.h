// File: kernel/exec/load.h
// Purpose: Load a userspace program
// SPDX-License-Identifier: MIT
#ifndef KERNEL_EXEC_LOAD_H
#define KERNEL_EXEC_LOAD_H

#include <kernel/exec/elf64.h> // for struct elf64_image
#include <kernel/mm/vm.h>      // for struct vm_root_pt

#include <sys/types.h> // for size_t

// User program loaded into userspace memory.
struct load_program {
	// The address at which userspace needs to jump.
	uintptr_t entry;

	// The user-page table root.
	struct vm_root_pt root;
};

// Loads a parsed ELF64 image into RAM.
int64_t load_elf64(struct load_program *prog, struct elf64_image *image);

#endif // KERNEL_EXEC_LOAD_H
