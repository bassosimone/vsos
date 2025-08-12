// File: kernel/mm/vm.c
// Purpose: Virtual memory manager.
// SPDX-License-Identifier: MIT

#include <kernel/boot/boot.h>   // for __kernel_base
#include <kernel/core/printk.h> // for printk
#include <kernel/mm/mm.h>       // for mm_map_identity
#include <kernel/mm/vm.h>       // for __vm_direct_map
#include <kernel/trap/trap.h>   // for trap_init_mm
#include <kernel/tty/uart.h>    // for uart_init_mm

#include <sys/types.h> // for uintptr_t

uintptr_t __vm_direct_map(uintptr_t phys_addr) {
	// TODO(bassosimone): implement direct mapping
	return phys_addr;
}

void __vm_map_kernel_memory(uintptr_t root_table) {
	printk("vm: mapping __kernel_base %llx -> __kernel_end %llx\n", __kernel_base, __kernel_end);
	mm_map_identity(root_table, (uint64_t)__kernel_base, (uint64_t)__kernel_end, MM_FLAG_EXEC);

	printk("vm: mapping __rodata_base %llx -> __rodata_end %llx\n", __rodata_base, __rodata_end);
	mm_map_identity(root_table, (uint64_t)__rodata_base, (uint64_t)__rodata_end, 0);

	printk("vm: mapping __data_base %llx -> __data_end %llx\n", __data_base, __data_end);
	mm_map_identity(root_table, (uint64_t)__data_base, (uint64_t)__data_end, MM_FLAG_WRITE);

	printk("vm: mapping __bss_base %llx -> __bss_end %llx\n", __bss_base, __bss_end);
	mm_map_identity(root_table, (uint64_t)__bss_base, (uint64_t)__bss_end, MM_FLAG_WRITE);

	printk("vm: mapping __stack_bottom %llx -> __stack_top %llx\n", __stack_bottom, __stack_top);
	mm_map_identity(root_table, (uint64_t)__stack_bottom, (uint64_t)__stack_top, MM_FLAG_WRITE);
}

void __vm_map_devices(void) {
	trap_init_mm();
	uart_init_mm();
}
