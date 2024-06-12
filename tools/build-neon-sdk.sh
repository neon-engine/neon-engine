#!/usr/bin/env bash

set -euo pipefail

BINUTILS_VERSION=2.42
GCC_VERSION=13.3.0
GLIBC_VERSION=2.28
KERNEL_MAJOR=5
KERNEL_MINOR=15
KERNEL_PATCH=160
GDB_VERSION=14.1

ARCH=x86_64

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
    --arch ) The architecture to build the SDK for, supported values are: x86_64, arm64
EOF
}

if [[ ! -d ./sdk ]]; then
  echo "no such sdk folder, make sure you are in the correct directory: <src>/tools/neon-sdk-builder.dockerfile"
  exit 2
fi

ERROR=
while true; do
  case "$1" in
    --help)
      help
      exit 0
      ;;
    --arch)
      ARCH=$2
      if [[ -z "${ARCH}" ]]; then
        echo "--arch needs a value, supported values are: x86_64, arm64"
        ERROR=true
      fi
      shift 2
      ;;
    *)
      break
      ;;
  esac
done

if [[ -n "${ERROR}" ]]; then
  exit 3
fi

podman build -t neon-sdk-builder -f neon-sdk-builder.dockerfile

TARGET_SDK_LOCATION="./sdk/neon-sdk-${ARCH}"

mkdir -p "${TARGET_SDK_LOCATION}"

podman run -i --rm -v "${TARGET_SDK_LOCATION}:/sdk:z" neon-sdk-builder:latest << EOF

  # download necessary tar files
  wget https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.gz
  wget https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.gz
  wget https://cdn.kernel.org/pub/linux/kernel/v${KERNEL_MAJOR}.x/linux-${KERNEL_MAJOR}.${KERNEL_MINOR}.${KERNEL_PATCH}.tar.xz
  wget https://ftp.gnu.org/gnu/glibc/glibc-${GLIBC_VERSION}.tar.gz
  wget https://ftp.gnu.org/gnu/gdb/gdb-${GDB_VERSION}.tar.gz

  tar zxf gcc-$GCC_VERSION.tar.gz
  mkdir gcc-build
  cd gcc-build
  ../gcc-$GCC_VERSION/configure \
    --enable-languages=c,c++ \
    --disable-multilib \
    --prefix=/sdk
  make -j$(nproc)
  make install
EOF
