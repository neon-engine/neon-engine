FROM --platform=linux/amd64 ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Los_Angeles

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
  libglfw3-dev \
  libglm-dev \
  && apt clean

ENTRYPOINT [ "/bin/bash" ]

