# BIOS PXE Debug Pipe

This debug pipe backend sends kernel log output as **raw Ethernet frames**
with a custom Zambesii EtherType over the PXE UNDI firmware path on IBM PC
BIOS builds.

## Protocol

| Field | Value |
|-------|--------|
| EtherType | `0x02B2` (default; `CONFIG_DEBUGPIPE_BIOS_PXE_ETHERTYPE`) |
| Destination MAC | configured at build time (no ARP) |
| Source MAC | locally administered `02:5a:6d:62:73:69` |
| Payload | Zbz frame: magic `Zb`, version `0x01`, type, body |

**Frame types:** `0x01` SESSION_START (once per boot), `0x02` LOG (printf text).

## Configure

Create a dedicated out-of-tree build directory:

```sh
mkdir -p b-pxe
cd b-pxe
../configure CFLAGS=-g CXXFLAGS=-g --host=i686-linux-gnu \
  ZCHIPSET=pc ZFIRMWARE=bios \
  --enable-scaling=ccnuma --with-max-ncpus=64 \
  --disable-kernel-vaddrspace-demand-paging \
  --disable-heap-demand-paging \
  --disable-debug-interrupts \
  --enable-dtrib-cisternn \
  --enable-debug-pipe-device-bios-pxe \
  --with-debugpipe-device-bios-pxe-target-mac=2c:cf:67:94:ab:cb \
  --with-debugpipe-device-bios-pxe-ethertype=0x02B2
make -j"$(nproc)"
```

The destination MAC must match the L2 address of the logging host. IP address
and UDP port are not used.

## Host logging server

Use the C++23 **`zbz-pxe-log-sink`** tool in
[`build/zbz-pxe-log-sink/`](../build/zbz-pxe-log-sink/). Full build, CLI,
systemd, and Pi deployment steps are in
[`build/zbz-pxe-log-sink/README.md`](../build/zbz-pxe-log-sink/README.md).

Quick start after building the subproject:

```sh
sudo ./build/zbz-pxe-log-sink/zbz-pxe-log-sink --interface auto \
  --log-dir /var/log/zbz-kernel
```

## QEMU notes

- Use an i386 machine with a PXE-capable NIC ROM (for example `e1000`).
- The guest NIC must reach the logging host at L2 (tap/bridge to `10.42.0.0/x`
  or similar; user-mode `10.0.2.0/24` will not reach a Pi on another segment).
- UNDI availability depends on firmware/ROM behavior.
- If UNDI is unavailable, the device fails closed and debug output continues
  through already-tied sinks (buffer/terminal/serial).
