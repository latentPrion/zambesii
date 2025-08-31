# Freestanding libstdc++ for Zambesii Kernel

libstdc++ (GCC's C++ standard library) configured for freestanding use with the
Zambesii kernel. Configure and make complete successfully on a fresh checkout.

## Quick Start

```bash
# Out-of-tree build (recommended)
mkdir b-cxx && cd b-cxx
../__kcore/__kstdlib/gcclibstdcxx/configure --host=i586-elf --prefix=/path/to/install
make -j$(nproc)
```

## Build Products

After a successful `make`, the following static libraries are produced:

| Library | Location | Purpose |
|---------|----------|---------|
| libsupc++.a | libstdcxx-v3/libsupc++/.libs/ | Exception handling, RTTI, memory operators |
| libsupc++convenience.a | libstdcxx-v3/libsupc++/.libs/ | Convenience lib (uninstalled) |
| libc++98convenience.a | libstdcxx-v3/src/c++98/.libs/ | C++98 runtime components |
| libc++11convenience.a | libstdcxx-v3/src/c++11/.libs/ | C++11 runtime components |
| libc++17convenience.a | libstdcxx-v3/src/c++17/.libs/ | C++17 runtime components |
| libc++20convenience.a | libstdcxx-v3/src/c++20/.libs/ | C++20 runtime components |
| libc++23convenience.a | libstdcxx-v3/src/c++23/.libs/ | C++23 runtime components |
| libc++26convenience.a | libstdcxx-v3/src/c++26/.libs/ | C++26 runtime components |
| libstdc++.a | libstdcxx-v3/src/.libs/ | Full static library |
| libstdc++convenience.a | libstdcxx-v3/src/.libs/ | Convenience lib (uninstalled) |
| libstdcxx-freestanding.a | (top level) | Intended combined lib; *may not be well-formed* |

**Note:** `libstdcxx-freestanding.a` is produced but may not yet be a properly
linked archive. Use the individual `.a` files from libsupc++ and src as needed.
Linkage against the kernel binary has not been verified.

## Prerequisites

The `gcclibstdcxx/` directory is a minimal mirror of the GCC source tree — all
of its contents are required. libstdc++v3's configure and build scripts assume
they live inside a full GCC checkout; this layout tricks them into behaving
correctly. Source the structure from a GCC tree (see `docs/3rdParty/`); notable
dependencies include `ltmain.sh`, `config.rpath`, `contrib/relpath.sh`, and
`libbacktrace/filetype.awk`.

## libstdcxx-v3 Version

The current `libstdcxx-v3/` subfolder is sourced from GCC 15.2.0 (latest
supported release at time of writing). GCC 14.2.0 was also building
successfully in the past. We are fine with freezing the kernel's libstdcxx
version indefinitely; upstream libstdc++ is stable for freestanding use and
we do not expect meaningful changes for our use case.

## Configure Flags

The build passes these key flags to libstdcxx-v3:

- `--enable-static` `--disable-shared` — Static libraries only
- `--disable-hosted-libstdcxx` — Freestanding target
- `--without-headers` — No system headers
- `--enable-libstdcxx-static-eh-pool` — Static EH pool for early boot
- `--with-libstdcxx-eh-pool-obj-count=64` — EH pool size

## Build System Structure

- **configure.ac** — Drives configuration; patches libstdcxx-v3 configure for
  ltmain.sh path; configures libstdcxx-v3 subdirectory via AX_SUBDIRS_CONFIGURE
- **patches/02-freestanding-makefile.patch** — Applied at configure time (`|| true`)
- **Makefile.am** — Builds libstdcxx-freestanding.a (links libsupc++convenience.la)

## Known Issues / TODO

1. **libstdcxx-freestanding.a** — May not be a well-formed archive; needs
   investigation
2. **Kernel linkage** — Unverified whether libsupc++ etc. link correctly with
   the kernel binary
3. **Integration** — gcclibstdcxx is not yet wired into the main Zambesii
   build system

## Compiler

Build with the i586-elf cross-compiler (or other `--host` as needed). The
cross-compiler should be configured with `--without-headers` and
`--disable-hosted-libstdcxx` for freestanding use.

