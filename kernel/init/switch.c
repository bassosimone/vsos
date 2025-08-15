// File: kernel/init/switch.c
// Purpose: Switch from kernel space to user space
// SPDX-License-Identifier: MIT

#include <kernel/core/assert.h>  // for KERNEL_ASSERT
#include <kernel/exec/elf64.h>   // for elf64_parse
#include <kernel/exec/load.h>    // for load_elf64
#include <kernel/init/initrd.h>  // for initrd_load
#include <kernel/init/switch.h>  // the subsystem's API
#include <kernel/sched/sched.h>  // for sched_process_start

[[noreturn]] void switch_to_userspace(void) {
	// 1. load the initial ramdisk
	struct initrd_info rd_info = {.base = 0, .count = 0};
	int rc = initrd_load(&rd_info);
	KERNEL_ASSERT(rc == 0);
	KERNEL_ASSERT(rd_info.base > 0 && rd_info.count > 0);

	// 2. interpret the ramdisk as an ELF binary
	struct elf64_image image = {
	    0,
	};
	rc = elf64_parse(&image, (const void *)rd_info.base, rd_info.count);
	KERNEL_ASSERT(rc == 0);

	// 3. load the ELF binary into memory
	struct load_program program = {
	    0,
	};
	rc = load_elf64(&program, &image);
	KERNEL_ASSERT(rc == 0);

	// 4. the super geronimo thing: return to userspace
	rc = sched_process_start(&program);
	KERNEL_ASSERT(rc >= 0);
	panic("unreachable code");
}
