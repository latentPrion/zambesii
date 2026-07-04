# zbz-pxe-log-sink

Host-side listener for Zambesii kernel debug output sent over a custom
Ethernet EtherType from the BIOS PXE UNDI debug pipe.

## Protocol

| Layer | Value |
|-------|--------|
| EtherType | `0x02B2` (default; kernel `CONFIG_DEBUGPIPE_BIOS_PXE_ETHERTYPE`) |
| Payload magic | ASCII `Zb` (`0x5A 0x62`) |
| Frame version | `0x01` |

**Message types** (byte after magic + version):

| Type | Name | Action |
|------|------|--------|
| `0x01` | SESSION_START | Open a new session log file |
| `0x02` | LOG | Append payload bytes to the current session file |

The kernel sends SESSION_START once per power cycle before orientation logs.
If LOG arrives with no open session, a fallback session file is created.

## Prerequisites

- Linux with `AF_PACKET` / `SOCK_RAW`
- `CAP_NET_RAW` (systemd `AmbientCapabilities` or root)
- C++23 compiler: g++ ≥ 13 or clang ≥ 16

## Build

From an out-of-tree Zambesii build directory (after top-level `configure`):

```sh
cd b
../configure …   # kernel options as needed
make -C build/zbz-pxe-log-sink -j"$(nproc)"
```

The binary is `b/build/zbz-pxe-log-sink/zbz-pxe-log-sink`.

Standalone configure inside the subproject also works:

```sh
cd build/zbz-pxe-log-sink
autoreconf -fi
./configure
make -j"$(nproc)"
```

## Run

```sh
sudo ./zbz-pxe-log-sink \
  --interface eth0 \
  --log-dir /var/log/zbz-kernel \
  --ethertype 0x02B2 \
  --idle-gap 30
```

### Options

| Flag | Default | Description |
|------|---------|-------------|
| `--interface` | `auto` | Interface name, or `auto` (first IPv4 on `10.42.0.0/24`) |
| `--log-dir` | `/var/log/zbz-kernel` | Directory for per-session log files |
| `--ethertype` | `0x02B2` | L2 EtherType filter |
| `--idle-gap` | `30` | Start new session after N seconds idle; `0` disables |
| `--raw` | off | Append LOG payloads without timestamp prefix |
| `--reopen-delay` | `1500` | Milliseconds before reopening socket after errors |

Session files are named `zbz-kernel-YYYYMMDDTHHMMSS.log` (with `-NNN` suffix on collision).

## Kernel configure (lab example)

```sh
--enable-debug-pipe-device-bios-pxe \
  --with-debugpipe-device-bios-pxe-target-mac=2c:cf:67:94:ab:cb \
  --with-debugpipe-device-bios-pxe-ethertype=0x02B2
```

The destination MAC must be the L2 address of the machine running this sink.
No ARP is performed in the kernel path.

## systemd

```sh
cd build/zbz-pxe-log-sink
sudo ./install-systemd.sh
```

Or manually:

```sh
sudo install -m 755 zbz-pxe-log-sink /usr/local/bin/
sudo install -d -m 755 /var/log/zbz-kernel
sudo cp zbz-pxe-log-sink.service /etc/systemd/system/
# Edit ExecStart if the binary path or interface differs.
sudo systemctl daemon-reload
sudo systemctl enable --now zbz-pxe-log-sink
```

## Verification

```sh
sudo tcpdump -i eth0 -e ether proto 0x02b2
```

Boot the kernel guest on shared L2 (tap/bridge to the Pi segment, not QEMU
user-mode `10.0.2.0/24`). A new file should appear under `--log-dir` on each
SESSION_START frame; power-cycling the guest creates a second file while the
sink keeps running.

## Resilience

- Runs until SIGTERM/SIGINT; flushes the current session on exit.
- Reopens the raw socket after interface or recv errors without exiting.
- `Restart=always` in the shipped unit covers process crashes.

See also [`docs/bios-pxe-debug-pipe.md`](../../docs/bios-pxe-debug-pipe.md) for
kernel-side notes.
