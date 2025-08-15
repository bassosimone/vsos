# Design

## Memory Layout
- **Kernel**: Identity-mapped at physical addresses (e.g., 0x40080000+)
- **User processes**: Virtual memory starting at 0x1000000
- **User stack**: Located at 0x2000000-0x2040000

## Privilege Levels (ARM64)
- **EL1 (Kernel)**: Handles system calls, interrupts, memory management
- **EL0 (User)**: Runs user processes with restricted access

## Page Table Strategy
- **Kernel page table**: Identity maps all RAM and devices
- **User page tables**: Include both user mappings AND kernel mappings
- **Syscall handling**: Switches from user PT → kernel PT → back to user PT
- **Security**: User processes cannot access kernel memory (enforced by page permissions)

## Process and Threading Model
- **Single-threaded processes**: One kernel thread per user process
- **ELF loading**: Dynamic process creation from embedded ELF binary
- **Memory isolation**: Each process has its own virtual address space
- **Statically allocated pool**: Fixed array of `MAX_THREADS` thread slots
- **Thread IDs as indices**: TID directly indexes into thread array (0 to max-1)
- **8KB kernel stacks**: Each thread has dedicated statically-allocated stack
- **Thread lifecycle**: UNUSED → RUNNABLE → BLOCKED/EXITED → UNUSED

## Scheduling Strategy
- **Cooperative in kernel**: Threads yield voluntarily, no kernel preemption
- **Timer-driven user preemption**: Clock interrupts trigger rescheduling on return to userspace
- **Round-robin fairness**: Fair scheduling using rotating thread cursor
- **Event-driven blocking**: Threads block on bitmask channels, awakened by events
- **Kernel-Thread context switching**: Preserves only ARM64 callee-saved registers for efficiency

## System Call and Trap Flow
1. User process executes `svc` instruction (syscall) or interrupt occurs
2. CPU traps to EL1, saves full user context (816-byte trap frame)
3. Kernel switches to kernel page table for security
4. Kernel handles syscall/interrupt with access to both user and kernel memory
5. On return to userspace: check for reschedule, restore user page table, return to EL0
6. **No nested interrupts**: IRQs disabled throughout handler execution

## Error Handling Strategy
- **Negative errno returns**: Functions return -ERRNO on error, ≥0 on success
- **Assertion-based invariants**: KERNEL_ASSERT for internal consistency checks
- **Panic on corruption**: Unrecoverable errors trigger kernel panic
- **No graceful degradation**: Fail-fast approach for hobby OS simplicity

## Build and Code Organization
- **Freestanding C23**: No standard library dependencies, custom libc subset
- **Machine-dependent split**: `_arm64` suffix for architecture-specific code
- **Embedded userspace**: Shell binary included via `.incbin` assembly directive
- **Ninja build system**: Explicit dependency tracking with parallel compilation
- **Clean module boundaries**: Subsystems follow `<module>_function_name` convention
- **Header strategy**: System headers (`include/`) vs kernel-internal headers
- **Minimal dependencies**: Clean separation between boot/mm/sched/trap/syscall
- **Flat hierarchy**: Avoid deep nesting, function names match file purpose
