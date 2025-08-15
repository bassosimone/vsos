// File: kernel/init/switch.h
// Purpose: Switch from kernel space to user space
// SPDX-License-Identifier: MIT
#ifndef KERNEL_INIT_SWITCH_H
#define KERNEL_INIT_SWITCH_H

// Switches from kernel space to user space by loading and starting init.
[[noreturn]] void switch_to_userspace(void);

#endif // KERNEL_INIT_SWITCH_H
