FROM almalinux:9

# Install everything in a single layer
RUN dnf update -y && \
    dnf install -y epel-release dnf-plugins-core && \
    dnf config-manager --set-enabled crb && \
    dnf groupinstall -y "Development Tools" && \
    dnf install -y \
        git \
        cmake \
        ninja-build \
        clang \
        llvm \
        llvm-devel \
        clang-devel \
        vulkan-headers \
        vulkan-loader-devel \
        vulkan-tools \
        vulkan-validation-layers \
        spirv-tools \
        glslang \
        mesa-vulkan-drivers \
        python3 \
        python3-pip && \
    clang --version && \
    cmake --version && \
    ninja --version && \
    vulkaninfo --summary && \
    dnf clean all

# Set environment variables for Vulkan
ENV VULKAN_SDK_PATH="/usr" \
    VK_LAYER_PATH="/usr/share/vulkan/explicit_layer.d"

# Set working directory
WORKDIR /workspace

# Set default command
CMD ["/bin/bash"]
