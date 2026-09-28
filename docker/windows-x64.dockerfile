# Windows x64 build environment for Neon Engine.
#
# Windows containers need a Windows host, so this is a Linux image that
# cross-compiles for Windows with llvm-mingw: clang 20 + libc++ + mingw-w64
# UCRT. Same compiler and C++ runtime as the Linux and macOS builds.
#
# Build the image (from the repository root):
#   docker build --platform linux/amd64 -t neon-engine/windows-x64 -f docker/windows-x64.dockerfile docker
#
# Compile the project inside it:
#   docker run --rm --platform linux/amd64 -v "$PWD":/src -w /src neon-engine/windows-x64 \
#     sh -c 'cmake --preset windows-x64-debug && cmake --build --preset windows-x64-debug'

FROM ubuntu:24.04

# llvm-mingw release tag. 20250709 ships LLVM 20.1.8.
ARG LLVM_MINGW_TAG=20250709
# Set automatically by BuildKit to amd64 or arm64; picks the host-native tarball.
ARG TARGETARCH
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates curl xz-utils git \
        cmake ninja-build make \
    && rm -rf /var/lib/apt/lists/*

RUN case "${TARGETARCH}" in \
        amd64) host_arch=x86_64 ;; \
        arm64) host_arch=aarch64 ;; \
        *) echo "unsupported TARGETARCH: ${TARGETARCH}" >&2; exit 1 ;; \
    esac \
    && curl -fsSL "https://github.com/mstorsjo/llvm-mingw/releases/download/${LLVM_MINGW_TAG}/llvm-mingw-${LLVM_MINGW_TAG}-ucrt-ubuntu-22.04-${host_arch}.tar.xz" \
        | tar -xJ -C /opt \
    && mv "/opt/llvm-mingw-${LLVM_MINGW_TAG}-ucrt-ubuntu-22.04-${host_arch}" /opt/llvm-mingw

ENV LLVM_MINGW_ROOT=/opt/llvm-mingw \
    PATH=/opt/llvm-mingw/bin:${PATH}

WORKDIR /src
