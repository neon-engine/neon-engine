#!/usr/bin/env bash

set -euo pipefail

BINUTILS_VERSION=2.42
GCC_VERSION=13.3.0
GLIBC_VERSION=2.28
KERNEL_MAJOR=5
KERNEL_MINOR=15
KERNEL_PATCH=160
GDB_VERSION=14.1

BUILD_ARCH=x86_64
BUILD_TARGET=linux

trap 'echo "build interrupted"; exit' SIGINT

OPTIONS=$(getopt -o "" --long "help,arch:,force" -n "$0" -- "$@")
# shellcheck disable=SC2181
if [ $? -ne 0 ]; then
  echo "error setting OPTIONS"
  exit 1
fi

eval set -- "${OPTIONS}"

function help() {
  cat << EOF
  Description: Builds cross-compilation SDK for C/C++
  Usage:
    ${0} [options]

  options:
    --help ) Displays this dialog
    --arch ) The architecture to build the SDK for, supported values are: x86_64
    --force ) Forces the script to remove SDK if it exists and rebuild it
EOF
}

function ask_yes_or_no() {
    read -p "$1 ([yY]es or [nN]o): "
    case $(echo $REPLY | tr '[A-Z]' '[a-z]') in
        y|yes) echo "yes" ;;
        n|no)  echo "no"  ;;
        *)     echo "?"   ;;
    esac
}

if [[ ! -d ./sdk ]]; then
  echo "no such sdk folder, make sure you are in the correct directory: <src>/tools/neon-sdk-builder.dockerfile"
  exit 2
fi

ERROR=
FORCE_REBUILD=
while true; do
  case "$1" in
    --help)
      help
      exit 0
      ;;
    --arch)
      BUILD_ARCH=$2
      if [[ -z "${BUILD_ARCH}" ]]; then
        echo "--arch needs a value, supported values are: x86_64"
        ERROR=true
      fi
      shift 2
      ;;
    --force)
      FORCE_REBUILD=1
      shift 1
      ;;
    --)
      shift
      break
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

TARGET_SDK_LOCATION="./sdk/neon-sdk.${BUILD_ARCH}-${BUILD_TARGET}"
DOWNLOAD_REPO="./sdk/download"



if [[ -d "${TARGET_SDK_LOCATION}" ]]; then
  for (( i = 0; i <= 3; i++ ))
  do
    if [[ "${FORCE_REBUILD}" = "1" ]]; then
      echo "forcing removal of ${TARGET_SDK_LOCATION}, and rebuilding it"
      rm -rf "${TARGET_SDK_LOCATION}"
      break
    fi
    if [[ "$i" = 3 ]]; then
      echo "could not get a response from the user"
      exit 4
    fi
    response=$(ask_yes_or_no "Found ${TARGET_SDK_LOCATION}, would you like to rebuild it? ")
    if [[ "${response}" = "no" ]]; then
      echo "Exiting script, not rebuilding ${TARGET_SDK_LOCATION}"
      exit 0
    elif [[ "${response}" = "yes" ]]; then
      rm -rf "${TARGET_SDK_LOCATION}"
      break
    else
      echo "please answer [yY]es or [nN]o"
    fi
  done
fi
mkdir -p "${TARGET_SDK_LOCATION}"
mkdir -p "${DOWNLOAD_REPO}"

podman run -i --rm -v "${TARGET_SDK_LOCATION}:/sdk:z" -v "${DOWNLOAD_REPO}:/download:z" neon-sdk-builder:latest << EOF
  trap 'exit 0' SIGINT

  set -euo pipefail

  BINUTILS_BIN=binutils-${BINUTILS_VERSION}.tar.gz
  GCC_BIN=gcc-${GCC_VERSION}.tar.gz
  LINUX_BIN=linux-${KERNEL_MAJOR}.${KERNEL_MINOR}.${KERNEL_PATCH}.tar.xz
  GLIBC_BIN=glibc-${GLIBC_VERSION}.tar.gz
  GDB_BIN=gdb-${GDB_VERSION}.tar.gz

  # download necessary tar files
  [[ ! -f /download/\${BINUTILS_BIN} ]] && wget -P /download https://ftp.gnu.org/gnu/binutils/\${BINUTILS_BIN}
  [[ ! -f /download/\${GCC_BIN}      ]] && wget -P /download https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/\${GCC_BIN}
  [[ ! -f /download/\${LINUX_BIN}    ]] && wget -P /download https://cdn.kernel.org/pub/linux/kernel/v${KERNEL_MAJOR}.x/\${LINUX_BIN}
  [[ ! -f /download/\${GLIBC_BIN}    ]] && wget -P /download https://ftp.gnu.org/gnu/glibc/\${GLIBC_BIN}
  [[ ! -f /download/\${GDB_BIN}      ]] && wget -P /download https://ftp.gnu.org/gnu/gdb/\${GDB_BIN}

  # build binutils first
  echo "building binutils"
  mkdir -p /build/binutils
  cd /build/binutils
  tar zxf /download/\${BINUTILS_BIN}
  mkdir binutils-build
  cd binutils-build
  ../binutils-${BINUTILS_VERSION}/configure \
    --target=${BUILD_ARCH}-${BUILD_TARGET} \
    --disable-multilib \
    --prefix=/sdk
  make -j$(nproc)
  make install

  # kernel headers second
  echo "installing linux headers"
  mkdir -p /build/linux
  cd /build/linux
  tar Jxf /download/\${LINUX_BIN}
  cd linux-${KERNEL_MAJOR}.${KERNEL_MINOR}.${KERNEL_PATCH}
  SHORT_ARCH=
  if [[ "${BUILD_ARCH}" = "x86_64" ]]; then
    SHORT_ARCH=x86
  fi
  make ARCH=\${SHORT_ARCH} INSTALL_HDR_PATH=/sdk/${BUILD_ARCH}-${BUILD_TARGET} headers_install

  # build gcc third
  echo "building gcc"
  mkdir -p /build/gcc
  cd /build/gcc
  tar zxf /download/\${GCC_BIN}
  mkdir gcc-build
  cd gcc-build
  ../gcc-${GCC_VERSION}/configure \
    --target=${BUILD_ARCH}-${BUILD_TARGET} \
    --enable-languages=c,c++ \
    --disable-multilib \
    --prefix=/sdk
  make -j$(nproc) all-gcc
  make install-gcc

  # build glibc fourth
  echo "building glibc"
  mkdir -p /build/glibc
  cd /build/glibc
  tar zxf /download/\${GLIBC_BIN}
  mkdir glibc-build
  cd glibc-build
  ../glibc-${GLIBC_VERSION}/configure \
    --prefix=/sdk/${BUILD_ARCH}-${BUILD_TARGET} \
    --build=\${MACHTYPE} \
    --host=${BUILD_ARCH}-${BUILD_TARGET} \
    --target=${BUILD_ARCH}-${BUILD_TARGET} \
    --with-headers=/sdk/${BUILD_ARCH}-${BUILD_TARGET}/include \
    --disable-multilib \
    --disable-werror \
    libc_cv_forced_unwind=yes
  make install-bootstrap-headers=yes install-headers
  make -j$(nproc) csu/subdir_lib
  install csu/crt1.o csu/crti.o csu/crtn.o /sdk/${BUILD_ARCH}-${BUILD_TARGET}/lib
  ${BUILD_ARCH}-${BUILD_TARGET}-gcc -nostdlib -nostartfiles -shared -x c /dev/null -o /sdk/${BUILD_ARCH}-${BUILD_TARGET}/lib/libc.so
  touch /sdk/${BUILD_ARCH}-${BUILD_TARGET}/include/gnu/stubs.h

  # build compiler support
  echo "adding compiler support"
  cd /build/gcc/gcc-build
  make -j$(nproc) all-target-libgcc
  make install-target-libgcc

  # compiler support library
  echo "adding compiler support library"
  cd /build/glibc/glibc-build
  make -j$(nproc)
  make install

  # standard library
  echo "adding standard library"
  cd /build/gcc/gcc-build
  make -j$(nproc)
  make install
  cd ..
EOF
