# Building Zambesii

This document outlines how to build the Zambesii kernel from source.

## Prerequisites

- Git
- **A GCC cross-compiler configured for i586-elf target** (see [CROSS-COMPILE.md](CROSS-COMPILE.md) if you need to build one)
- cdrtools or cdrkit (mkisofs or genisoimage)
- autoconf, automake, and libtool
- GNU Make and Bash

Set your PATH to include where you built the cross-compiler:

```bash
export PATH="~/toolchain/bin:$PATH"
```

## Getting the Source

Clone the Zambesii repository:

```bash
git clone https://github.com/latentPrion/zambesii-git.git
cd zambesii-git
```

## Building

### Configuration

Zambesii uses a standard autotools build system. The build requires several parameters to be specified:

- `--host`: The cross-compilation target (usually i586-elf)
- `ZCHIPSET`: The chipset to target (ibmPc or generic)
- `ZFIRMWARE`: The firmware to use (ibmPcBios, uefi, or generic)

For a typical build with debug symbols, run:

```bash
./configure \
    CFLAGS=-g \
    CXXFLAGS=-g \
    --host=i586-elf \
    ZCHIPSET=pc \
    ZFIRMWARE=bios \
    --enable-scaling=ccnuma \
    --with-max-ncpus=64
```

Run `./configure --help` to see all available options.

### Compilation

After configuration, run:

```bash
make
```

This will build the kernel binary file `zambesii.zxe`.

To create an ISO image, run one of the following commands:

```bash
# For GRUB legacy
make grub-iso

# For GRUB2
make grub2-iso
```

The ISO files will be created in the following locations:
- GRUB legacy: `./packaging/iso/grub/zambesii.iso`
- GRUB2: `./packaging/iso/grub2/zambesii.iso`

## Testing

You can test the kernel with QEMU using either the kernel binary directly or the ISO:

### Using the kernel binary directly:

```bash
qemu-system-i386 -kernel zambesii.zxe -m 256M -smp 6 -serial stdio
```

### Using the ISO file:

```bash
qemu-system-i386 -cdrom packaging/iso/grub/zambesii.iso -m 256M -smp 6 -serial stdio
```

### Testing NUMA support:

For testing NUMA configurations:

```bash
qemu-system-i386 -kernel zambesii.zxe -m 256M -smp 6 \
  -object memory-backend-ram,id=ram0,size=128M \
  -object memory-backend-ram,id=ram1,size=128M \
  -numa node,nodeid=0,cpus=0-1,memdev=ram0 \
  -numa node,nodeid=1,cpus=2-3,memdev=ram1 \
  -serial stdio
```

Or with an ISO:

```bash
qemu-system-i386 -cdrom packaging/iso/grub/zambesii.iso -m 256M -smp 6 \
  -object memory-backend-ram,id=ram0,size=128M \
  -object memory-backend-ram,id=ram1,size=128M \
  -numa node,nodeid=0,cpus=0-1,memdev=ram0 \
  -numa node,nodeid=1,cpus=2-3,memdev=ram1 \
  -serial stdio
```