# Building a Cross-Compiler for Zambesii

This guide explains how to build a GCC cross-compiler targeting i586-elf, which is required to build the Zambesii kernel.

## Prerequisites

- Standard UNIX utilities like wget/curl and tar
- An existing working GCC installation
- GNU Make
- Bison, Flex, and Texinfo

**Note: It's recommended to use a recent GCC version.** Having matching versions of GCC and your cross-compiler will minimize any potential problems.

## Setting Up Directories

Decide where you're going to put your cross-compiler, e.g. `~/toolchain/i586-elf`. This will be your $PREFIX for installing GCC and binutils.

```bash
export TARGET=i586-elf
mkdir -p ~/toolchain/$TARGET/build/{binutils,gcc,gmp,mpc,mpfr}
export PREFIX=~/toolchain/$TARGET
export SRC=~/toolchain/src
mkdir -p $SRC
```

## Download and Unpack Sources

A working GNU cross-compiler toolchain needs 5 packages:

- Binutils: http://ftp.gnu.org/gnu/binutils/
- GCC: ftp://ftp.gnu.org/gnu/gcc/
- GMP: ftp://ftp.gnu.org/gnu/gmp/
- MPC: ftp://ftp.gnu.org/gnu/mpc/
- MPFR: ftp://ftp.gnu.org/gnu/mpfr/

For example (replace VERSION with the latest versions):

```bash
cd $SRC
curl -L ftp://ftp.gnu.org/gnu/binutils/binutils-2.40.tar.bz2 | tar -xf -
curl -L ftp://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.gz | tar -xf -
curl -L ftp://ftp.gnu.org/gnu/gmp/gmp-6.3.0.tar.xz | tar -xf -
curl -L ftp://ftp.gnu.org/gnu/mpc/mpc-1.3.1.tar.gz | tar -xf -
curl -L ftp://ftp.gnu.org/gnu/mpfr/mpfr-4.2.1.tar.xz | tar -xf -
```

## Building the Cross-Compiler

### Binutils

```bash
cd ~/toolchain/$TARGET/build/binutils
$SRC/binutils-VERSION/configure --target=$TARGET --prefix=$PREFIX --disable-nls --with-sysroot
make -j$(nproc)
make install
```

### GMP, MPC, and MPFR

These libraries are required by GCC:

```bash
# GMP
cd ~/toolchain/$TARGET/build/gmp
$SRC/gmp-VERSION/configure --prefix=$PREFIX
make -j$(nproc)
make install

# MPC
cd ~/toolchain/$TARGET/build/mpc
$SRC/mpc-VERSION/configure --prefix=$PREFIX --with-gmp=$PREFIX
make -j$(nproc)
make install

# MPFR
cd ~/toolchain/$TARGET/build/mpfr
$SRC/mpfr-VERSION/configure --prefix=$PREFIX --with-gmp=$PREFIX
make -j$(nproc)
make install
```

### GCC

```bash
cd ~/toolchain/$TARGET/build/gcc
$SRC/gcc-VERSION/configure --target=$TARGET --prefix=$PREFIX \
  --disable-nls --enable-languages=c,c++ --without-headers \
  --with-gmp=$PREFIX --with-mpc=$PREFIX --with-mpfr=$PREFIX

make -j$(nproc) all-gcc
make -j$(nproc) all-target-libgcc
make install-gcc
make install-target-libgcc
```

## Verification

If all went well, `$PREFIX/bin/$TARGET-gcc --version` should print out version information about your cross-compiler.

Now you can add the cross-compiler to your PATH and proceed to build Zambesii:

```bash
export PATH="$PREFIX/bin:$PATH"
```

Return to [BUILD.md](BUILD.md) to continue with building Zambesii.