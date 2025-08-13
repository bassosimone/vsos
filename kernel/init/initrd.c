// File: kernel/init/initrd.c
// Purpose: Reading the initial ramdisk.
// SPDX-License-Identifier: MIT

#include <kernel/boot/boot.h>   // for __shell_start
#include <kernel/core/assert.h> // for KERNEL_ASSERT
#include <kernel/core/printk.h> // for printk
#include <kernel/init/initrd.h> // for initrd_data

#include <sys/types.h> // for size_t

static inline uintptr_t __initrd_data(void) {
	return (uintptr_t)__shell_start;
}

static inline size_t __initrd_size(void) {
	return (uintptr_t)__shell_end - (uintptr_t)__shell_start;
}

int64_t initrd_load(struct initrd_info *info) {
	KERNEL_ASSERT(info != 0);
	info->base = __initrd_data();
	info->count = __initrd_size();
	printk("initrd: loaded %lld bytes\n", info->count);
	return 0;
}
