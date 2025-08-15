# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

VSOS (Very Small Operating System) is a minimal, hobby operating system written in C targeting ARM64 architecture. It runs exclusively on `qemu-system-aarch64 -M virt` and implements basic OS functionality including virtual memory, system calls, process management, and a simple shell.

## Build Commands

```bash
# Build the kernel and embedded shell
ninja

# Clean all built artifacts
ninja -t clean
```

The build system uses ninja and produces `kernel.elf` (the kernel) with the shell ELF binary embedded inside it.

## Testing and Debugging

```bash
# Run the OS in QEMU (basic)
qemu-system-aarch64 -M virt -cpu cortex-a53 -nographic -kernel kernel.elf

# Run with detailed debugging logs
qemu-system-aarch64 -M virt -cpu cortex-a53 -d in_asm,cpu,int,guest_errors -D qemu.log -nographic -kernel kernel.elf

# Disassemble the kernel for debugging
llvm-objdump -d --no-show-raw-insn -M no-aliases kernel.elf

# View ELF headers and sections
readelf -a kernel.elf
```

## Architecture Overview

### Memory Layout
- **Kernel**: Identity-mapped at physical addresses (starting around 0x40080000)
- **User processes**: Virtual memory starting at 0x1000000
- **User stack**: Located at 0x2000000-0x2040000

### Privilege Levels (ARM64)
- **EL1 (Kernel)**: Handles system calls, interrupts, memory management
- **EL0 (User)**: Runs user processes with restricted access

### Key Subsystems
- **kernel/boot/**: Boot sequence and kernel initialization
- **kernel/mm/**: Physical page allocator (`page.c`) and virtual memory management (`vm.c`, `vm_arm64.c`)
- **kernel/sched/**: Cooperative multitasking scheduler with kernel threads
- **kernel/syscall/**: System call implementation (currently `SYS_read` and `SYS_write`)
- **kernel/trap/**: Exception and interrupt handling for ARM64
- **kernel/exec/**: ELF64 binary loader for user processes
- **libc/**: Minimal C library (separate builds for kernel and user space)

### System Call Flow
1. User process executes `svc` instruction
2. CPU traps to EL1, saves user context
3. Kernel switches to kernel page table
4. Kernel handles syscall with access to both user and kernel memory
5. Kernel restores user page table and returns to EL0

### Process Model
- Single-threaded processes with one kernel thread per user process
- ELF loading from embedded initrd
- Memory isolation through separate virtual address spaces
- Page table strategy: User page tables include both user mappings AND kernel mappings

## Code Conventions

- Use clang with `-std=c23` for both kernel and user space
- Target: `aarch64-none-elf` (freestanding)
- Kernel code is built with `-DARCH_ARM64`
- Headers are split between system headers (`include/`) and kernel-internal headers
- File naming: `*_arm64.c` for architecture-specific code, `*.S` for assembly
- Error handling: Use negative errno values for errors, non-negative for success
- Thread IDs are used as array indices (0 to SCHED_MAX_THREADS-1)

## Key Implementation Details

- The kernel uses cooperative multitasking with a round-robin scheduler
- Page allocation uses a bitmap-based allocator
- The shell is embedded as binary data in the kernel and loaded at boot
- System calls are limited to read/write operations currently
- All code is freestanding (no standard library dependencies)
- The build process creates both kernel and user space versions of libc functions