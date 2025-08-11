# MiniOS: Very Small Operating System

This repository contains a really-basic, portable, hobby operating system.

Portions of this code derive from [nuta/operating-system-in-1000-lines](
https://github.com/nuta/operating-system-in-1000-lines).

## Roadmap

- [x] bootstrap with `qemu-system-aarch -M virt`(arm64)
- [x] return from interrupt at EL1 context (arm64)
- [x] console driver (PL011 UART)
- [x] `printk` (arm64)
- [x] cooperative multitasking (arm64)
- [x] system-timer driver (arm64)
- [x] virtual memory at EL1 context (arm64)
- [x] interruptible process sleep (arm64)
- [x] bitmap based allocator
- [ ] `SYS_read` (arm64) - IN PROGRESS
- [ ] `SYS_write` (arm64) - IN PROGRESS
- [ ] initial shell (arm64) - IN PROGRESS
- [ ] ELF loader (arm64) - IN PROGRESS
- [ ] virtual memory at EL0 context (arm64)
- [ ] zygote user process (arm64)
- [ ] `O_NONBLOCK` and `SYS_select` (arm64)
- [ ] block device driver (arm64)
- [ ] file system (arm64)

## Dependencies

On Ubuntu 25.04, we need these dependencies:

```bash
sudo apt install build-essential clang clangd lld llvm qemu-system-arm
```

## Building

```bash
ninja
```

## Cleaning

```bash
ninja -t clean
```

## Testing

```bash
qemu-system-aarch64 \
  -M virt \
  -cpu cortex-a53 \
  -nographic \
  -kernel kernel.elf
```

Add `-d in_asm,cpu,int,guest_errors -D qemu.log` to investigate errors.

## Dumping

```bash
llvm-objdump -d --no-show-raw-insn -M no-aliases kernel.elf
```

or:

```bash
readelf -a kernel.elf
```

## License

```
SPDX-License-Identifier: MIT
```
