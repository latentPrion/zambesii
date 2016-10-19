- [Prerequisites](#orgheadline1)
- [Setup Directories](#orgheadline2)
- [Download and Unpack Sources](#orgheadline3)
- [Build!](#orgheadline9)
  - [Binutils](#orgheadline4)
  - [GMP](#orgheadline5)
  - [MPC](#orgheadline6)
  - [MPFR](#orgheadline7)
  - [GCC](#orgheadline8)
- [Next: Building Zambesii](#orgheadline10)


# Prerequisites<a id="orgheadline1"></a>

-   Standard UNIX utilities like wget/curl and tar
-   An existing working GCC installation
-   GNU Make
-   Bison, Flex, and Texinfo; packages vary depending on OS and distro.

**Note: It might be a good idea to check that your GCC version is up-to-date.** Having matching versions of GCC and your cross-compiler will minimize any potential problems.

# Setup Directories<a id="orgheadline2"></a>

Decide where you're going to put your cross-compiler, e.g. `~/toolchain/i586-elf`. This will be your $PREFIX for installing GCC and binutils. Set your $TARGET variable to i586-elf and make some directories to hold build output:

    export TARGET=i586-elf
    mkdir ~/toolchain && mkdir ~/toolchain/$TARGET && cd ~/toolchain/$TARGET
    mkdir build && mkdir build/binutils && mkdir build/gcc && mkdir build/gmp && mkdir build/mpc && mkdir build/mpfr

Now point $PREFIX at your toolchain directory:

    export PREFIX=~/toolchain/$TARGET

# Download and Unpack Sources<a id="orgheadline3"></a>

A working GNU cross-compiler toolchain needs 5 packages:

-   Binutils: <http://ftp.gnu.org/gnu/binutils/>
-   GCC: <ftp://ftp.gnu.org/gnu/gcc/>
-   GMP: <ftp://ftp.gnu.org/gnu/gmp/>
-   MPC: <ftp://ftp.gnu.org/gnu/mpc/>
-   MPFR: <ftp://ftp.gnu.org/gnu/mpfr/>

You'll want the latest versions of each, unless your system GCC is severely out of date. Download and unpack the tarballs:

    mkdir ~/toolchain/src && cd ~/toolchain/src
    curl -L ftp://ftp.gnu.org/gnu/binutils/binutils-VERSION.tar.bz2 | tar -xf -
    curl -L ftp://ftp.gnu.org/gnu/gcc/gcc-VERSION/gcc-VERION.tar.bz2 | tar -xf -
    curl -L ftp://ftp.gnu.org/gnu/gmp/gmp-VERSION.tar.bz2 | tar -xf -
    curl -L ftp://ftp.gnu.org/gnu/mpc/mpc-VERSION.tar.gz | tar -xf -
    curl -L ftp://ftp.gnu.org/gnu/mpfr/mpfr-VERSION.tar.bz2 | tar -xf -

In each command, replace VERSION with the latest version of the package.

Note: You don't need to put these in your $TARGET directory. You can build multiple cross-compilers from the same source.

You might want to export $SRC to save some typing below:

    export SRC=~/toolchain/src

# Build!<a id="orgheadline9"></a>

At this point you just have to configure-make-install and wait for a while. Be sure to build and install GMP, MPC, and MPFR before building GCC.

Note: You don't have to run `make install` as root since your $PREFIX is writable by you.

**Non-Linux users:** use `gmake` instead of `make` for the commands below.

**Tip:** Use -jN in the `make` commands below to use N CPU cores. Usually N should be the number of physical cores in your machine ± 1.

## Binutils<a id="orgheadline4"></a>

    cd ~/toolchain/$TARGET/build/binutils
    $SRC/binutils-VERSION/configure --target=$TARGET --prefix=$PREFIX --disable-nls --with-sysroot
    make all
    make install

## GMP<a id="orgheadline5"></a>

    cd ~/toolchain/$TARGET/build/gmp
    $SRC/gmp-VERSION/configure --prefix=$PREFIX
    make
    make install

## MPC<a id="orgheadline6"></a>

    cd ~/toolchain/$TARGET/build/mpc
    $SRC/mpc-VERSION/configure --prefix=$PREFIX
    make
    make install

## MPFR<a id="orgheadline7"></a>

    cd ~/toolchain/$TARGET/build/mpfr
    $SRC/mpfr-VERSION/configure --prefix=$PREFIX
    make
    make install

## GCC<a id="orgheadline8"></a>

Now that we've got everything else ready, time to build GCC. This could take quite a while depending on your hardware. Don't forget to use -j<threads> to speed things up (I forgot to do this…).

GCC has a lot of options. Run `$SRC/gcc-VERSION/configure --help` to get an idea for what you can do.

    cd ~/toolchain/$TARGET/build/gcc
    $SRC/gcc-VERSION/configure --target=$TARGET --target=$TARGET --prefix=$PREFIX \
      --disable-nls --enable-languages=c,c++ --without-headers \
      --with-gmp=$PREFIX --with-mpc=$PREFIX --with-mpfr=$PREFIX
    
    make all-gcc
    make all-target-libgcc
    make install-gcc
    make install-target-libgcc

# Next: Building Zambesii<a id="orgheadline10"></a>

If all went well, `~$PREFIX/bin/$TARGET-gcc --version` should print out some info about your cross-compiler.

Now you can move on to building the Zambesii kernel: see build.md in this repository.
