# MiniOS: Very Small Operating System

This repository contains a really-basic, portable, hobby operating
system. Part of the codebase is derived from [nuta/operating-system-in-1000-lines](
https://github.com/nuta/operating-system-in-1000-lines).

## Roadmap

- [x] bootstrap with `qemu-system-aarch -M virt`(arm64)
- [x] return from interrupt at EL1 context (arm64)
- [x] console driver (PL011 UART)
- [x] `printk` (arm64)
- [x] cooperative multitasking (arm64)
- [x] system-timer driver (arm64)
- [x] virtual-memory at EL1 context (arm64)
- [ ] `SYS_read` (arm64) - IN PROGRESS
- [ ] `SYS_write` (arm64) - IN PROGRESS
- [ ] initial shell (arm64) - IN PROGRESS
- [ ] ELF loader (arm64) - IN PROGRESS
- [ ] zygote user process
- [ ] `O_NONBLOCK` and `SYS_select`
- [ ] block device driver
- [ ] file system

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
