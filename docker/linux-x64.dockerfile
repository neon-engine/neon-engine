# Linux x64 build environment for Neon Engine.
#
# Toolchain: LLVM 20 (clang, lld, lldb, libc++) on Ubuntu 24.04, pinned so the
# compiler and C++ runtime match the Windows and macOS builds.
#
# Build the image (from the repository root):
#   docker build --platform linux/amd64 -t neon-engine/linux-x64 -f docker/linux-x64.dockerfile docker
#
# Compile the project inside it:
#   docker run --rm --platform linux/amd64 -v "$PWD":/src -w /src neon-engine/linux-x64 \
#     sh -c 'cmake --preset linux-x64-debug && cmake --build --preset linux-x64-debug'

FROM ubuntu:24.04

ARG LLVM_VERSION=20
ENV DEBIAN_FRONTEND=noninteractive

# Base tooling plus the system libraries SDL2 needs to build its X11, Wayland,
# OpenGL, and audio backends from source.
#
# glslang compiles the Vulkan shaders during the build. The Vulkan loader and
# Mesa's lavapipe, a software Vulkan driver, let the container render headless
# without a GPU.
RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates curl gnupg git \
        cmake ninja-build make pkg-config \
        glslang-tools \
        libvulkan1 mesa-vulkan-drivers \
        libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev \
        libxinerama-dev libxss-dev libxkbcommon-dev \
        libwayland-dev wayland-protocols libdecor-0-dev \
        libgl1-mesa-dev libegl1-mesa-dev libgles2-mesa-dev libdrm-dev libgbm-dev \
        libdbus-1-dev libudev-dev libasound2-dev libpulse-dev \
    && rm -rf /var/lib/apt/lists/*

# LLVM from the official apt repository, pinned to a single major version.
RUN curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key \
        | gpg --dearmor -o /usr/share/keyrings/llvm.gpg \
    && echo "deb [signed-by=/usr/share/keyrings/llvm.gpg] http://apt.llvm.org/noble/ llvm-toolchain-noble-${LLVM_VERSION} main" \
        > /etc/apt/sources.list.d/llvm.list \
    && apt-get update && apt-get install -y --no-install-recommends \
        clang-${LLVM_VERSION} lld-${LLVM_VERSION} lldb-${LLVM_VERSION} \
        libc++-${LLVM_VERSION}-dev libc++abi-${LLVM_VERSION}-dev \
        clang-format-${LLVM_VERSION} clang-tidy-${LLVM_VERSION} \
    && rm -rf /var/lib/apt/lists/* \
    && for tool in clang clang++ lld ld.lld lldb clang-format clang-tidy; do \
           update-alternatives --install /usr/bin/${tool} ${tool} /usr/bin/${tool}-${LLVM_VERSION} 100; \
       done

ENV CC=clang \
    CXX=clang++

WORKDIR /src
