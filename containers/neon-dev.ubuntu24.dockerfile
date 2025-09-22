FROM ubuntu:24.04

# Install everything in a single layer
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        git \
        cmake \
        ninja-build \
        gdb lldb \
        clang \
        llvm \
        llvm-dev \
        libclang-dev \
        libvulkan-dev \
        vulkan-tools \
        vulkan-utility-libraries-dev \
        spirv-tools \
        glslang-tools \
        mesa-vulkan-drivers \
        mesa-utils \
        python3 \
        python3-pip && \
    clang --version && \
    cmake --version && \
    ninja --version && \
    vulkaninfo --summary && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Set environment variables for Vulkan
ENV VULKAN_SDK_PATH="/usr" \
    VK_LAYER_PATH="/usr/share/vulkan/explicit_layer.d"

