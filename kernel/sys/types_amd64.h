// File: kernel/sys/types_amd64.h
// Purpose: types specific to the AMD64 architecture.
// SPDX-License-Identifier: MIT
// Adapted from: https://github.com/nuta/operating-system-in-1000-lines
#ifndef KERNEL_SYS_TYPES_AMD64_H
#define KERNEL_SYS_TYPES_AMD64_H

// Integer types on the amd64 architecture.
typedef unsigned long long size_t;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned long long uintptr_t;

// Limits definitions on the amd64 architecture.
#define UINTPTR_MAX 0xFFFFFFFFFFFFFFFFULL
#define SIZE_MAX UINTPTR_MAX

#endif // KERNEL_SYS_TYPES_AMD64_H
