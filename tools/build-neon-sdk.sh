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

EOF
