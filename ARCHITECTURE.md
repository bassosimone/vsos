# Architecture

- [build.ninja](build.ninja) build script

- [include](include) contains system and libc headers

- [kernel](kernel) contains the kernel

    - [kernel/asm](kernel/asm) contains C wrappers for assembly routines

    - [kernel/boot](kernel/boot) boot code and linker script

    - [kernel/clock](kernel/clock) code to program periodic timers

    - [kernel/core](kernel/core) core functionality (e.g., `KERNEL_ASSERT`)

    - [kernel/drivers](kernel/drivers) device drivers

    - [kernel/exec](kernel/exec) load and execute ELF64 binaries

    - [kernel/init](kernel/init) implements invoking the first userspace process

    - [kernel/mm](kernel/mm) physical and virtual memory management

    - [kernel/sched](kernel/sched) scheduler for kernel threads

    - [kernel/syscall](kernel/syscall) implements syscalls

    - [kernel/trap](kernel/trap) handles traps (e.g., interrupts and syscalls)

    - [kernel/tty](kernel/tty) serial port interface

- [libc](libc) minimal C library

- [shell](shell) minimal userspace shell
