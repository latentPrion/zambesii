#!/bin/bash

# Script to configure the kernel with specific options
# Created: $(date)

# Exit on error
set -e

echo "Running autoreconf..."
autoreconf -i .. ../build/zudi

HOST_TRIPLET="${HOST_TRIPLET:-i586-elf}"
CPPFLAGS="${CPPFLAGS}"
CFLAGS="${CFLAGS:--g}"
CXXFLAGS="${CXXFLAGS:--g}"
LDFLAGS="${LDFLAGS}"
if [ -z "${REPLACE_ARGS}" ]
then REPLACE_ARGS=$(cat <<'EOF'
    --disable-rt-kernel-irqs
    --enable-all-debug-opts
 --disable-kernel-vaddrspace-demand-paging
 --disable-heap-demand-paging
 --disable-debug-interrupts
    --enable-dtrib-cisternn
EOF
)
fi
APPEND_ARGS="${APPEND_ARGS:- }"

echo "Running configure with custom options..."
../configure \
    CPPFLAGS="${CPPFLAGS}" \
    CFLAGS="${CFLAGS}" \
    CXXFLAGS="${CXXFLAGS}" \
    LDFLAGS="${LDFLAGS}" \
    --host="${HOST_TRIPLET}" \
    ZCHIPSET=pc \
    ZFIRMWARE=bios \
    --enable-scaling=ccnuma \
    --with-max-ncpus=64 \
    ${REPLACE_ARGS} ${APPEND_ARGS}

echo "Configuration completed successfully!"
