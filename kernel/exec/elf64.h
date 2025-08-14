// File: kernel/exec/elf64.h
// Purpose: Code for parsing ELF64 binaries.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_EXEC_ELF64_H
#define KERNEL_EXEC_ELF64_H

#include <sys/types.h> // for size_t

// Loads an ELF64 file into memory and returns the entry point.
int64_t elf64_load(uintptr_t *entry, const void *data, size_t size);

#endif // KERNEL_EXEC_ELF64_H
