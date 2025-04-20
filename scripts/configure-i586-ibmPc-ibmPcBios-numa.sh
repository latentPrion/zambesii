#!/bin/bash

# Script to configure the kernel with specific options
# Created: $(date)

# Exit on error
set -e

echo "Running autoreconf..."
autoreconf ..

echo "Running configure with custom options..."
../configure \
    CFLAGS=-g \
    CXXFLAGS=-g \
    --host=i586-elf \
    ZCHIPSET=pc \
    ZFIRMWARE=bios \
    --enable-scaling=ccnuma \
    --with-max-ncpus=64 \
 --disable-rt-kernel-irqs \
    --enable-all-debug-opts \
 --disable-kernel-vaddrspace-demand-paging \
 --enable-debug-page-faults \
 --disable-debug-interrupts \
 --enable-debug-lock-exceptions \
 --enable-debug-locks \
    --enable-dtrib-cisternn

echo "Configuration completed successfully!"
