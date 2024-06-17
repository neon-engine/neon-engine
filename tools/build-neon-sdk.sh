#!/usr/bin/env bash

set -euo pipefail

BUILD_ARCH=x86_64
BUILD_TARGET=linux
TOOLCHAIN_REPO=https://github.com/neon-engine/buildroot
TOOLCHAIN_BRANCH=2024.02.x

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
    read -rp "$1 [Yn]: "
    case $(echo "$REPLY" | tr '[A-Z]' '[a-z]') in
        y|yes) echo "yes" ;;
        n|no)  echo "no"  ;;
        *)     echo "yes"   ;;
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

TARGET_SDK_LOCATION="./sdk/neon-sdk.${BUILD_ARCH}-${BUILD_TARGET}"

echo "Building SDK for:"
echo "Platform: ${BUILD_TARGET}"
echo "Arch: ${BUILD_ARCH}"
echo "SDK will be deployed to: ${TARGET_SDK_LOCATION}"
echo ""

if [[ -z "${FORCE_REBUILD}" ]]; then
  response=$(ask_yes_or_no "Would you like to proceed?")
  if [[ "${response}" = "no" ]]; then
    echo "Exiting script, not rebuilding ${TARGET_SDK_LOCATION}"
    exit 4
  fi
fi

if [[ -d "${TARGET_SDK_LOCATION}" ]] && [[ -n "${FORCE_REBUILD}" ]]; then
  echo "forcing removal of ${TARGET_SDK_LOCATION}, and rebuilding it"
  rm -rf "${TARGET_SDK_LOCATION}"
fi

if [[ -d "${TARGET_SDK_LOCATION}" ]]; then
  response=$(ask_yes_or_no "Found ${TARGET_SDK_LOCATION}, would you like to rebuild it?")
  if [[ "${response}" = "no" ]]; then
    echo "Exiting script, not rebuilding ${TARGET_SDK_LOCATION}"
    exit 5
  else
    rm -rf "${TARGET_SDK_LOCATION}"
  fi
fi

mkdir -p "${TARGET_SDK_LOCATION}"

if [[ "${BUILD_TARGET}" == "linux" ]]; then
  podman build -t neon-sdk-builder -f neon-sdk-builder.dockerfile
  podman run -i --rm \
    -v "${TARGET_SDK_LOCATION}:/sdk:z" \
    -e FORCE_UNSAFE_CONFIGURE=1 \
    -e HOME=/home \
    neon-sdk-builder:latest -c "
      git clone -b ${TOOLCHAIN_BRANCH} ${TOOLCHAIN_REPO} /build;
      cd /build;
      cp neon-${BUILD_ARCH}.config .config;
      make syncconfig;
      make sdk;
      tar xzf \"/build/output/images/${BUILD_ARCH}-neon-linux-gnu_sdk-buildroot.tar.gz\" -C /sdk;
    "
fi
