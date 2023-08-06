FROM --platform=linux/amd64 ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get -y install tzdata

# Install dev tools from apt, git, and pipx
RUN apt update \
    && apt install -y \
          python3 \
          git \
          clang \
          gdb \
          cmake \
          make \
          ninja-build \
          libgl1-mesa-dev \
          vulkan-tools \
          libvulkan-dev \
          vulkan-validationlayers-dev \
          spirv-tools \
          build-essential \
          tar \
          curl \
          zip \
          unzip \
          libxinerama-dev \
          libxcursor-dev \
          xorg-dev \
          libglu1-mesa-dev \
          pkg-config \
    && apt clean \
    && git clone https://github.com/skaslev/gl3w \
    && cd gl3w \
    && python3 gl3w_gen.py --root /usr/local \
    && cd .. \
    && rm -r gl3w

ENTRYPOINT [ "/bin/bash" ]
