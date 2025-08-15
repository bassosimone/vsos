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

## System Call Flow
1. User process executes `svc` instruction
2. CPU traps to EL1, saves user context
3. Kernel switches to the kernel page table
4. Kernel handles syscall with access to both user and kernel memory
5. Kernel restores user page table and returns to EL0

## Process Model
- **Single-threaded processes**: One kernel thread per user process
- **ELF loading**: Dynamic process creation from embedded ELF binary
- **Memory isolation**: Each process has its own virtual address space
