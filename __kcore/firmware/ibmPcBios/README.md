# IBM PC BIOS PXE debug pipe (`ibmPcBiosPxe`)

This directory contains `ibmPcBiosPxe.cpp`, a debug-pipe backend that sends
kernel log output as raw Ethernet frames (custom EtherType) through the PXE
UNDI stack. See also [`docs/bios-pxe-debug-pipe.md`](../../../docs/bios-pxe-debug-pipe.md)
for configure flags, frame format, and host-side capture.

## Why x86Emu cannot load PXE today

PXE discovery in `ibmPcBiosPxe.cpp` follows two paths:

1. Scan guest low memory for a literal `!PXE` structure.
2. If none is found, locate a NIC option ROM with a PXE/UNDI header and invoke
   its **UNDI loader** entry point through the in-kernel **x86Emu** BIOS
   emulator (`ibmPcBiosPxe_runEmuAt()`).

On typical QEMU setups with SeaBIOS and an iPXE NIC ROM (for example
`pxe-e1000.rom`), path 1 fails and path 2 is attempted — but path 2 also
fails. **We cannot use x86Emu to bring up PXE/UNDI from these ROMs.**

### Compressed iPXE option ROMs

Modern iPXE images shipped as PCI option ROMs are **LZMA-compressed**. The
on-disk ROM file does not contain a literal `!PXE` signature; that structure
is created only after the ROM's real-mode **decompress / install** path runs.

SeaBIOS POST executes only the iPXE **prefix `init`** hook. It does **not**
run the **`exec`** / BEV path that decompresses the main image into low memory.
When the Zambesii kernel starts (via GRUB/multiboot), **`!PXE` is not present
in guest RAM** even though the UNDI header is visible in the uncompressed ROM
prefix.

### x86Emu cannot run the loader

The UNDI loader and BEV/exec stubs must execute as **real-mode x86 code** on
the physical CPU: they manipulate segment registers, call into option-ROM
thunks, and (for iPXE) run **`install_prealloc`** LZMA decompression.

x86Emu is a simplified real-mode interpreter used elsewhere in this firmware
layer for isolated BIOS calls. It does **not** faithfully emulate everything
those ROM entry points require. In practice:

- `undiloader` returns with `pxeSeg == 0` and `pxeOff == 0` (no PXE pointer).
- The BEV/exec fallback also fails to publish `!PXE` in low memory.

So the emulator path is a dead end for LZMA-compressed iPXE ROMs. The device
**fails closed**: if UNDI is unavailable, debug output continues through sinks
that were already tied (buffer, terminal, serial).

## What would be needed instead

A **real-mode CPU thunk** — drop to real mode on the boot CPU, far-call the ROM
entry from a low-memory trampoline (similar in spirit to
`__kcpuPowerOnEntry.S`), then return to protected/long mode — is the viable
approach for actually installing iPXE and obtaining a working UNDI transmit
path.

Until that exists, treat this implementation as **protocol and wiring complete,
runtime PXE bring-up not functional** under SeaBIOS + compressed iPXE ROMs.
