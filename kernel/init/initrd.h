// File: kernel/init/initrd.h
// Purpose: Reading the initial ramdisk.
// SPDX-License-Identifier: MIT
#ifndef KERNEL_INIT_INITRD_H
#define KERNEL_INIT_INITRD_H

#include <sys/types.h> // for size_t

// Information about the loaded initial ramdisk.
struct initrd_info {
	uintptr_t base;
	size_t count;
};

// Loads the initial ramdisk or returns a negative errno.
int64_t initrd_load(struct initrd_info *info);

#endif // KERNEL_INIT_INITRD_H
