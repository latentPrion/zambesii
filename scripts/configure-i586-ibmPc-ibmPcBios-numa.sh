#!/bin/bash

# Script to configure the kernel with specific options
# Created: $(date)

# Exit on error
set -e

echo "Running autoreconf..."
autoreconf -i .. ../build/zudi

echo "Running configure with custom options..."
../configure \
    CFLAGS=-g \
    CXXFLAGS=-g \
    --host=i586-elf \
    ZCHIPSET=pc \
    ZFIRMWARE=bios \
    --enable-scaling=ccnuma \
    --with-max-ncpus=64 \
    --enable-all-debug-opts \
    --enable-dtrib-cisternn

echo "Configuration completed successfully!" 
