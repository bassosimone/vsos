# Operating System

This repository contains a really-basic, portable operating
system derived from [nuta/operating-system-in-1000-lines](
https://github.com/nuta/operating-system-in-1000-lines).

## Dependencies

On Ubuntu 25.04, we need these dependencies to compile the kernel:

``` bash
sudo apt install build-essential clang clangd lld llvm
```

## Building

``` bash
ninja
```

## Testing

``` bash
qemu-system-aarch64 \                                                                main!?
  -M virt \
  -cpu cortex-a53 \
  -nographic \
  -kernel kernel.elf
```

Add `-d in_asm,cpu,int,guest_errors -D qemu.log` to investigate errors.
