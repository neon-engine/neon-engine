#!/usr/bin/env bash

set -euo pipefail

BINUTILS_VERSION=2.42
GCC_VERSION=13.3.0
GLIBC_VERSION=2.28
KERNEL_MAJOR=5
KERNEL_MINOR=15
KERNEL_PATCH=160
GDB_VERSION=14.1

OPTIONS=$(getopt --long help,arch: -n "$0" -- "$@")
# shellcheck disable=SC2181
if [ $? -ne 0 ]; then
  exit 1
fi

eval set -- "${OPTIONS}"

help() {
  cat << EOF
  Description: Builds cross-compilation SDK for C/C++
  Usage:
    ${0} [options]

  options:
    --help ) Displays this dialog
    --arch ) The architecture to build the SDK for
EOF
}

if [[ ! -d /sdk ]]; then
  echo "no such /sdk, make sure you are in the correct environment"
  exit 2
fi

podman build -t neon-sdk-builder -f neon-sdk

wget https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.gz
wget https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.gz
wget https://cdn.kernel.org/pub/linux/kernel/v${KERNEL_MAJOR}.x/linux-${KERNEL_MAJOR}.${KERNEL_MINOR}.${KERNEL_PATCH}.tar.xz
wget https://ftp.gnu.org/gnu/glibc/glibc-${GLIBC_VERSION}.tar.gz
wget https://ftp.gnu.org/gnu/gdb/gdb-${GDB_VERSION}.tar.gz

tar zxf gcc-$GCC_VERSION.tar.gz
RUN mkdir gcc-build
RUN cd gcc-build
RUN ../gcc-$GCC_VERSION/configure \
  --enable-languages=c,c++ \
  --disable-multilib \
  --prefix=/sysroot
RUN make -j4
RUN make install